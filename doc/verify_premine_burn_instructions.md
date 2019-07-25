You can verify premine burn, using Ryo Wallet Atom and its view-only wallet features.

1. Download and install the latest verison or Ryo Wallet Atom: https://ryo-currency.com/wallet/#download
2. Download premine key image sets: https://github.com/ryo-currency/ryo-currency/blob/master/utils/burned_premine_keyimages/key_images.7z
3. Run Atom, choose "Restore view-only wallet".
4. When restoring view-only wallet, you will need wallet address + viewkey. Give any desired name for wallet, and set password if you wish (or hit "Enter" to set blank password). Use these addresses and viewkeys:

> Wallet 1: Address:
> SumonzyoAnidrp3B6WhbntJLRJsPRD7LiSd2e5HjRV9h6qWj6ia2ihgRXn8ZwmwfQgZ1mL9EkcsBcELL4h84v1yhMF9opLgjM1j
> Viewkey:
> e6e58faa93dba88315f67f53cba9413ebf0f423badb62696aa28ce6c0db2df00
> 
> Wallet 2: Address:
> SumonzyopdC7egt7MWhJcNfR3ktmfWqPNcu3Sb9aejAZ48W4a1ZW95vjXyQDuLLg2gSN8GkP3474R2wRr1h9ggy2BUHhbrLJJ51
> Viewkey:
> 25ba5fa5abb5f99962754001ff631e01684f9a59871d798a20a1658ba31c950c
> 
> Wallet 3: Address:
> SumonzyoK3hCffFQ8djaKZhokmPZsiuEaeLfEtnJfTXVQV3ce3mNHKHQ5kYLPVfNMsS7dhmyHihvhfKU6xXCrmaL4RVDo8eR3gB
> Viewkey:
> 1a8c55e52f7f80da3dfd2eadf71a74afb3242c20168ad888b57f483dbc38b00e
> 
> Dev Wallet: Address:
> Sumoo4aMxWbLgqVazxbiNNFSDyhBEonRf99KmceTEnYUCWUQR2gXF1617P8xQxaMcGi5BAU7juzThSTboV6e1gitSjkfjq2zgY2
> Viewkey:
> e12497b6dc6c2cbaf7b311e54b93e4e5f6367acda69a49f9ef20c51d9c689f00

5. After clicking "open wallet" you will have to wait for sync to finish (watch bottom bar statistics). When finished - you will see incoming transactions. Since viewkey we used to restore wallet, doesn't show outgoing transactions or burn - we need to import key images. Navigate to "Wallet actions" dropdown and select "Manage key images" (It will become active when the sync process finishes), choose "import" option and press "browse" to navigate appropriate key in step [1].
6. After import you will see the actual balance of wallet. In our case, since they are premine + dev wallets they will be close to 0 (barring some small donations).
![restore view-only wallet](https://i.imgur.com/tRlelTF.jpg)
![see incoming transfers](https://i.imgur.com/GolNK88.jpg)
![import key images to see final balance](https://i.imgur.com/kGlm4uq.jpg)
