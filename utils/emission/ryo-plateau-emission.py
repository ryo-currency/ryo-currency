#!/bin/python

from datetime import datetime
from math import floor

START_TIMESTAMP   = 1492486495
DIFFICULTY_TARGET = 240 # seconds

MONEY_SUPPLY          = 88888888000000000
EMISSION_SPEED_FACTOR = 19
FINAL_SUBSIDY         = 4000000000
GENESIS_BLOCK_REWARD  = 8800000000000000

PREMINE_BURN_AMOUNT = 8700051446427001

DEV_FUND_PERIOD = 15 * 24 * 7 # 1 week
DEV_FUND_AMOUNT = 8000000000000000
DEV_FUND_LENGTH = 52 * 6 # 6 years
DEV_FUND_START  = 161500

# coin emission change interval/speed configs
# THESE ARE THE ONLY NEW / MODIFIED CONSTANTS
COIN_EMISSION_MONTH_INTERVAL = 6. # months
HEIGHT_PER_MONTH_6_MONTH = int(COIN_EMISSION_MONTH_INTERVAL * (30.4375 * 24 * 3600) / DIFFICULTY_TARGET)
COIN_EMISSION_HEIGHT_INTERVAL  = HEIGHT_PER_MONTH_6_MONTH

HEIGHT_PER_YEAR = 2 * HEIGHT_PER_MONTH_6_MONTH
PEAK_COIN_EMISSION_YEAR = 2.5 # years
PEAK_COIN_EMISSION_LENGTH = 1 # years
PEAK_COIN_EMISSION_HEIGHT = HEIGHT_PER_YEAR * PEAK_COIN_EMISSION_YEAR
PEAK_COIN_EMISSION_HEIGHT_END = PEAK_COIN_EMISSION_HEIGHT + HEIGHT_PER_YEAR * PEAK_COIN_EMISSION_LENGTH

OUTPUT = "ryo-plateau.tsv"

def get_base_reward_compute(height, already_generated_coins):
    base_reward = 0
    round_factor = 10000000

    if height == 0:
        base_reward = GENESIS_BLOCK_REWARD

    elif height < PEAK_COIN_EMISSION_HEIGHT_END:
        """
        Here we use min(height, PEAK_COIN_EMISSION_HEIGHT) to limit the interval_num and create the plateau
        """
        interval_num = int(min(height, PEAK_COIN_EMISSION_HEIGHT) / COIN_EMISSION_HEIGHT_INTERVAL)
        money_supply_pct = 0.1888 + interval_num*(0.023 + interval_num*0.0032)
        base_reward = int(MONEY_SUPPLY * money_supply_pct) >> EMISSION_SPEED_FACTOR

    elif already_generated_coins < MONEY_SUPPLY:
        """
        Here interval_num is a zero based index for the interval after the plateau
        """
        interval_num = int((height - PEAK_COIN_EMISSION_HEIGHT_END) / COIN_EMISSION_HEIGHT_INTERVAL)
        base_reward = get_base_reward_compute(PEAK_COIN_EMISSION_HEIGHT, already_generated_coins)
        for n in range(interval_num + 1): # for(int n = 0; n < interval_num; n++)
            reward_decrease = int((base_reward * 653) / 10000)
            """
            Here we check to see if reward_decrease is applied if it is below tail reward
            If so, we need to not subtract it, because we are still in the phase where
            already_generated_coins < MONEY_SUPPLY
            """
            if base_reward - reward_decrease < FINAL_SUBSIDY:
                break
            base_reward -= reward_decrease

    else:
        base_reward = FINAL_SUBSIDY

    base_reward = int(base_reward / round_factor ) * round_factor

    return base_reward

    
def get_base_reward_tabular(height, already_generated_coins):
    BLOCK_REWARDS = [32000000000, 36450000000, 41970000000, 48590000000, 56280000000, 65070000000, 65070000000, 60820000000, 56840000000, 53130000000, 49660000000, 46420000000, 43390000000, 40550000000, 37910000000, 35430000000, 33120000000, 30950000000, 28930000000, 27040000000, 25280000000, 23630000000, 22080000000, 20640000000, 19290000000, 18030000000, 16850000000, 15750000000, 14720000000, 13760000000, 12860000000, 12020000000, 11240000000, 10500000000, 9820000000, 9180000000, 8580000000, 8020000000, 7490000000, 7000000000, 6540000000, 6120000000, 5720000000, 5340000000, 4990000000, 4670000000, 4360000000, 4080000000]

    """
    Here we get the interval_num and limit it to the last index of BLOCK_REWARDS
    """
    interval_num = min(int(height / COIN_EMISSION_HEIGHT_INTERVAL), len(BLOCK_REWARDS) - 1)

    if height == 0:
        base_reward = GENESIS_BLOCK_REWARD

    elif already_generated_coins < MONEY_SUPPLY:
        base_reward = BLOCK_REWARDS[interval_num]
        
    else:
        base_reward = FINAL_SUBSIDY

    return base_reward


def get_dev_fund_amount(height):

    amount = 0
    if height < DEV_FUND_START:
        return amount

    height -= DEV_FUND_START

    if height / DEV_FUND_PERIOD >= DEV_FUND_LENGTH:
        return amount

    if height % DEV_FUND_PERIOD != 0:
        return amount

    amount = int(DEV_FUND_AMOUNT / DEV_FUND_LENGTH)

    return amount
    
        
def calculate_emssion_speed():
    
    height = 0
    block_reward = 0
    already_generated_coins = 0
    circ_supply = 0
    circ_supply_at_tail = 0 # debug
    dev_fund = 0
    timestamp = START_TIMESTAMP
    
    f.write("height\tblock_reward\tcoin_emitted\temission_pct\tdev_fund\tdev_pct_emission\ttimestamp\tdate\n")

    print(COIN_EMISSION_HEIGHT_INTERVAL)
    while circ_supply < MONEY_SUPPLY + 500000: # loop through block 0 to a few years after tail 

        block_reward = get_base_reward_tabular(height, already_generated_coins)
        block_reward_compute = get_base_reward_compute(height, already_generated_coins)
        
        if block_reward != block_reward_compute:
            print("Error at height ", height, block_reward, block_reward_compute)
            exit()

        dev_block_reward = get_dev_fund_amount(height)

        if block_reward == 4000000000 and circ_supply_at_tail == 0:
            circ_supply_at_tail = circ_supply
            print("circ_supply_at_tail: ", '{0:.2f}'.format(circ_supply_at_tail / 1000000000.0))

        
        """
        dev_fund does not get added to already_generated_coins since the burned premine
        is still part of this sum
        """
        already_generated_coins += block_reward

        dev_fund += dev_block_reward
        
        circ_supply = already_generated_coins + dev_fund - PREMINE_BURN_AMOUNT

        # Print every month
        if int(height % floor(COIN_EMISSION_HEIGHT_INTERVAL/6)) == 0:
            _height = format(height, '07')
            _block_reward = '{0:.2f}'.format(block_reward/1000000000.0)
            _circ_supply = '{0:.2f}'.format(circ_supply/1000000000.0)
            _emission_pct = str(round(circ_supply*100.0/MONEY_SUPPLY, 2))
            _dev_fund = '{0:.2f}'.format(dev_fund/1000000000.0)
            _dev_emission_pct = str(round(dev_fund*100.0/circ_supply, 2))
            _timestamp = str(timestamp)
            _date = datetime.utcfromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
            
            f.write(str(height) + "\t" + str(block_reward) + "\t" + str(circ_supply) + "\t" + _emission_pct + "\t" + str(dev_fund) + "\t" + _dev_emission_pct + "\t" + _timestamp + "\t" + _date + "\n")

        
        timestamp += DIFFICULTY_TARGET
        height += 1
        
    
if __name__ == "__main__":
    f = open(OUTPUT, "w")
    calculate_emssion_speed()
    f.close()
