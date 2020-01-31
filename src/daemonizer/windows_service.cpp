// Copyright (c) 2019, Ryo Currency Project
// Portions copyright (c) 2014-2018, The Monero Project
//
// Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
// All rights reserved.
//
// Authors and copyright holders give permission for following:
//
// 1. Redistribution and use in source and binary forms WITHOUT modification.
//
// 2. Modification of the source form for your own personal use.
//
// As long as the following conditions are met:
//
// 3. You must not distribute modified copies of the work to third parties. This includes
//    posting the work online, or hosting copies of the modified work for download.
//
// 4. Any derivative version of this work is also covered by this license, including point 8.
//
// 5. Neither the name of the copyright holders nor the names of the authors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// 6. You agree that this licence is governed by and shall be construed in accordance
//    with the laws of England and Wales.
//
// 7. You agree to submit all disputes arising out of or in connection with this licence
//    to the exclusive jurisdiction of the Courts of England and Wales.
//
// Authors and copyright holders agree that:
//
// 8. This licence expires and the work covered by it is released into the
//    public domain on 1st of February 2020
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <boost/chrono/chrono.hpp>
#include <boost/thread/thread.hpp>

#undef UNICODE
#undef _UNICODE

#include "common/scoped_message_writer.h"
#include "daemonizer/windows_service.h"
#include "string_tools.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <shellapi.h>
#include <thread>
#include <utility>
#include <windows.h>

#include "common/gulps.hpp"

#define GULPS_PRINT_FAIL(...) GULPS_ERROR("Error: ", __VA_ARGS__)
#define GULPS_PRINT_OK(...) GULPS_PRINT(__VA_ARGS__)

namespace windows
{
GULPS_CAT_MAJOR("win_ser");
namespace
{
typedef std::unique_ptr<std::remove_pointer<SC_HANDLE>::type, decltype(&::CloseServiceHandle)> service_handle;

std::string get_last_error()
{
	LPSTR p_error_text = nullptr;

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&p_error_text), 0, nullptr);

	if(nullptr == p_error_text)
	{
		return "";
	}
	else
	{
		std::string ret(p_error_text);
		LocalFree(p_error_text);
		return ret;
	}
}

bool relaunch_as_admin(
	std::string const &command, std::string const &arguments)
{
	SHELLEXECUTEINFO info{};
	info.cbSize = sizeof(info);
	info.lpVerb = "runas";
	info.lpFile = command.c_str();
	info.lpParameters = arguments.c_str();
	info.hwnd = nullptr;
	info.nShow = SW_SHOWNORMAL;
	if(!ShellExecuteEx(&info))
	{
		GULPS_PRINT_FAIL("Admin relaunch failed: ", get_last_error());
		return false;
	}
	else
	{
		return true;
	}
}

// When we relaunch as admin, Windows opens a new window.  This just pauses
// to allow the user to read any output.
void pause_to_display_admin_window_messages()
{
	boost::chrono::milliseconds how_long{1500};
	boost::this_thread::sleep_for(how_long);
}
} // namespace

bool check_admin(bool &result)
{
	BOOL is_admin = FALSE;
	PSID p_administrators_group = nullptr;

	SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;

	if(!AllocateAndInitializeSid(
		   &nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &p_administrators_group))
	{
		GULPS_PRINT_FAIL("Security Identifier creation failed: ", get_last_error());
		return false;
	}

	if(!CheckTokenMembership(
		   nullptr, p_administrators_group, &is_admin))
	{
		GULPS_PRINT_FAIL("Permissions check failed: ", get_last_error());
		return false;
	}

	result = is_admin ? true : false;

	return true;
}

bool ensure_admin(
	std::string const &arguments)
{
	bool admin;

	if(!check_admin(admin))
	{
		return false;
	}

	if(admin)
	{
		return true;
	}
	else
	{
		std::string command = epee::string_tools::get_current_module_path();
		relaunch_as_admin(command, arguments);
		return false;
	}
}

bool install_service(
	std::string const &service_name, std::string const &arguments)
{
	std::string command = epee::string_tools::get_current_module_path();
	std::string full_command = command + arguments;

	service_handle p_manager{
		OpenSCManager(
			nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE),
		&::CloseServiceHandle};
	if(p_manager == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't connect to service manager: ", get_last_error());
		return false;
	}

	service_handle p_service{
		CreateService(
			p_manager.get(), service_name.c_str(), service_name.c_str(), 0
			//, GENERIC_EXECUTE | GENERIC_READ
			,
			SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, full_command.c_str(), nullptr, nullptr, ""
			//, "NT AUTHORITY\\LocalService"
			,
			nullptr // Implies LocalSystem account
			,
			nullptr),
		&::CloseServiceHandle};
	if(p_service == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't create service: ", get_last_error());
		return false;
	}

	GULPS_PRINT_FAIL("Service installed", get_last_error());

	pause_to_display_admin_window_messages();

	return true;
}

bool start_service(
	std::string const &service_name)
{
	GULPS_INFO("Starting service");

	SERVICE_STATUS_PROCESS service_status = {};
	DWORD unused = 0;

	service_handle p_manager{
		OpenSCManager(
			nullptr, nullptr, SC_MANAGER_CONNECT),
		&::CloseServiceHandle};
	if(p_manager == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't connect to service manager: ", get_last_error());
		return false;
	}

	service_handle p_service{
		OpenService(
			p_manager.get(), service_name.c_str()
			//, SERVICE_START | SERVICE_QUERY_STATUS
			,
			SERVICE_START),
		&::CloseServiceHandle};
	if(p_service == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't find service: ", get_last_error());
		return false;
	}

	if(!StartService(
		   p_service.get(), 0, nullptr))
	{
		GULPS_PRINT_FAIL("Service start request failed: ", get_last_error());
		return false;
	}

	GULPS_PRINT_FAIL("Service started", get_last_error());

	pause_to_display_admin_window_messages();

	return true;
}

bool stop_service(
	std::string const &service_name)
{
	GULPS_INFO("Stopping service");

	service_handle p_manager{
		OpenSCManager(
			nullptr, nullptr, SC_MANAGER_CONNECT),
		&::CloseServiceHandle};
	if(p_manager == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't connect to service manager: ", get_last_error());
		return false;
	}

	service_handle p_service{
		OpenService(
			p_manager.get(), service_name.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS),
		&::CloseServiceHandle};
	if(p_service == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't find service: ", get_last_error());
		return false;
	}

	SERVICE_STATUS status = {};
	if(!ControlService(p_service.get(), SERVICE_CONTROL_STOP, &status))
	{
		GULPS_PRINT_FAIL("Couldn't request service stop: ", get_last_error());
		return false;
	}

	GULPS_PRINT_FAIL("Service stopped", get_last_error());

	pause_to_display_admin_window_messages();

	return true;
}

bool uninstall_service(
	std::string const &service_name)
{
	service_handle p_manager{
		OpenSCManager(
			nullptr, nullptr, SC_MANAGER_CONNECT),
		&::CloseServiceHandle};
	if(p_manager == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't connect to service manager: ", get_last_error());
		return false;
	}

	service_handle p_service{
		OpenService(
			p_manager.get(), service_name.c_str(), SERVICE_QUERY_STATUS | DELETE),
		&::CloseServiceHandle};
	if(p_service == nullptr)
	{
		GULPS_PRINT_FAIL("Couldn't find service: ", get_last_error());
		return false;
	}

	SERVICE_STATUS status = {};
	if(!DeleteService(p_service.get()))
	{
		GULPS_PRINT_FAIL("Couldn't uninstall service: ", get_last_error());
		return false;
	}

	GULPS_PRINT_FAIL("Service uninstalled", get_last_error());

	pause_to_display_admin_window_messages();

	return true;
}

} // namespace windows
