#define NOMINMAX
#include <atlbase.h>
#include <Windows.h>
#include <comutil.h>
#include "mlapi_wrapper.h"
#include "../3rdparty/MysticLight_SDK/MysticLight_SDK.h"
#include <atlsafe.h>
#include <string>
#include <tuple>

// for _bstr_t
#ifdef _DEBUG
#pragma comment(lib, "comsuppwd.lib")
#else
#pragma comment(lib, "comsuppw.lib")
#endif

namespace starl1ght
{
	namespace mystic_light
	{
		// helpers
		result result_from_int(int mlapi_code)
		{
			switch (mlapi_code) {
			case 0: return result::ok;
			case -1: return result::generic_error;
			case -2: return result::timeout;
			case -3: return result::not_implemented;
			case -4: return result::not_initialized;
			case -101: return result::invalid_argument;
			case -102: return result::device_not_found;
			case -103: return result::not_supported;
			default: return result::not_in_enum;
			}
		}

		bool have_admin_rights()
		{
			BOOL fRet = FALSE;
			HANDLE hToken = NULL;
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
				TOKEN_ELEVATION Elevation;
				DWORD cbSize = sizeof(TOKEN_ELEVATION);
				if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
					fRet = Elevation.TokenIsElevated;
				}
			}
			if (hToken) {
				CloseHandle(hToken);
			}
			return fRet != FALSE;
		}

		// members
		static HMODULE light_dll{ nullptr };
		static LPMLAPI_Initialize MLAPI_Initialize{ nullptr };
		static LPMLAPI_GetDeviceInfo MLAPI_GetDeviceInfo{ nullptr };
		static LPMLAPI_GetLedColor MLAPI_GetLedColor{ nullptr };
		static LPMLAPI_GetLedStyle MLAPI_GetLedStyle{ nullptr };
		static LPMLAPI_GetLedBright MLAPI_GetLedBright{ nullptr };
		static LPMLAPI_GetLedMaxBright MLAPI_GetLedMaxBright{ nullptr };
		static LPMLAPI_GetLedSpeed MLAPI_GetLedSpeed{ nullptr };
		static LPMLAPI_GetLedMaxSpeed MLAPI_GetLedMaxSpeed{ nullptr };
		static LPMLAPI_GetLedInfo MLAPI_GetLedInfo{ nullptr };
		static LPMLAPI_SetLedBright MLAPI_SetLedBright{ nullptr };
		static LPMLAPI_SetLedStyle MLAPI_SetLedStyle{ nullptr };
		static LPMLAPI_SetLedSpeed MLAPI_SetLedSpeed{ nullptr };
		static LPMLAPI_SetLedColor MLAPI_SetLedColor{ nullptr };

		// actual interface
		result initialize(const wchar_t* path_to_dll)
		{
			if (light_dll) {
				return result::ok;
			}

			if (!have_admin_rights()) {
				return result::admin_rights_are_needed;
			}

			light_dll = LoadLibraryW(path_to_dll);
			if (!light_dll) {
				return result::invalid_argument;
			}

			const auto fail = [](result possible_result = result::generic_error) {
				FreeLibrary(light_dll);
				light_dll = nullptr;
				return possible_result;
			};

#ifndef STARL1GHT_ML_LOAD_FN
#define STARL1GHT_ML_LOAD_FN(fn_name) \
			fn_name = reinterpret_cast<LP##fn_name>(GetProcAddress(light_dll, #fn_name)); \
			if (!fn_name) return fail();
#endif

			STARL1GHT_ML_LOAD_FN(MLAPI_Initialize);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetDeviceInfo);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetLedColor);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetLedStyle);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetLedBright);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetLedMaxBright);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetLedSpeed);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetLedMaxSpeed);
			STARL1GHT_ML_LOAD_FN(MLAPI_GetLedInfo);
			STARL1GHT_ML_LOAD_FN(MLAPI_SetLedBright);
			STARL1GHT_ML_LOAD_FN(MLAPI_SetLedStyle);
			STARL1GHT_ML_LOAD_FN(MLAPI_SetLedSpeed);
			STARL1GHT_ML_LOAD_FN(MLAPI_SetLedColor);

#undef STARL1GHT_ML_LOAD_FN

			if (MLAPI_Initialize() != 0) {
				return fail();
			}
			return result::ok;
		}

		std::tuple<result, std::vector<std::wstring>, std::vector<uint32_t>> get_device_info()
		{
			if (!light_dll) {
				return{ result::not_initialized, {}, {} };
			}
			LPSAFEARRAY arr_devices, arr_leds;
			CComSafeArray<BSTR> raw_devices, raw_leds;

			const auto raw_result = MLAPI_GetDeviceInfo(&arr_devices, &arr_leds);
			raw_devices.Attach(arr_devices);
			raw_leds.Attach(arr_leds);

			const auto result = result_from_int(raw_result);

			if (result != result::ok) {
				return{ result, {}, {} };
			}

			std::vector<std::wstring> devices;
			std::vector<uint32_t> leds;

			for (uint32_t i = 0; i < raw_devices.GetCount(); ++i) {
				const BSTR bstr_dev = raw_devices.GetAt(i);
				devices.emplace_back(bstr_dev, SysStringLen(bstr_dev));

				const BSTR bstr_leds = raw_leds.GetAt(i);
				const std::wstring tmp(bstr_leds, SysStringLen(bstr_leds));
				leds.emplace_back(wcstoul(tmp.c_str(), nullptr, 10));
			}

			return{ result, devices, leds };
		}

		// 0 ... 255
		std::tuple<result, uint8_t, uint8_t, uint8_t> get_led_color(const wchar_t* device, uint32_t led_id)
		{
			if (!light_dll) {
				return{ result::not_initialized, 0, 0, 0 };
			}

			_bstr_t device_bstr{ device };
			DWORD r, g, b;

			const auto result = MLAPI_GetLedColor(device_bstr.GetBSTR(), led_id, &r, &g, &b);
			return{ result_from_int(result), static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b) };
		}

		std::tuple<result, std::wstring> get_led_style(const wchar_t* device, uint32_t led_id)
		{
			if (!light_dll) {
				return{ result::not_initialized, {} };
			}

			_bstr_t device_bstr{ device };
			_bstr_t style_bstr;

			const auto result = MLAPI_GetLedStyle(device_bstr.GetBSTR(), led_id, &style_bstr.GetBSTR());
			return{ result_from_int(result), std::wstring(style_bstr.GetBSTR(), style_bstr.length()) };
		}

		// 1 ... 5
		std::tuple<result, uint32_t> get_led_bright(const wchar_t* device, uint32_t led_id)
		{
			if (!light_dll) {
				return{ result::not_initialized, {} };
			}

			_bstr_t device_bstr{ device };
			DWORD bright;
			const auto result = MLAPI_GetLedBright(device_bstr.GetBSTR(), led_id, &bright);
			return{ result_from_int(result), bright };
		}

		std::tuple<result, uint32_t> get_led_max_bright(const wchar_t* device, uint32_t led_id)
		{
			return std::tuple<result, uint32_t>();
		}

		// 1 ... 3
		std::tuple<result, uint32_t> get_led_speed(const wchar_t* device, uint32_t led_id)
		{
			if (!light_dll) {
				return{ result::not_initialized, {} };
			}

			_bstr_t device_bstr{ device };
			DWORD speed;

			const auto result = MLAPI_GetLedSpeed(device_bstr.GetBSTR(), led_id, &speed);
			return{ result_from_int(result), speed };
		}

		std::tuple<result, uint32_t> get_led_max_speed(const wchar_t* device, uint32_t led_id)
		{
			return std::tuple<result, uint32_t>();
		}

		std::tuple<result> get_led_info(const wchar_t* device, uint32_t led_id)
		{
			return std::tuple<result>();
		}

		result set_led_style(const wchar_t* device, uint32_t led_id, const wchar_t* style)
		{
			if (!light_dll) {
				return result::not_initialized;
			}

			_bstr_t device_bstr{ device };
			_bstr_t style_bstr{ style };

			const auto result = MLAPI_SetLedStyle(device_bstr.GetBSTR(), led_id, style_bstr.GetBSTR());
			return result_from_int(result);
		}

		result set_led_color(const wchar_t* device, uint32_t led_id, uint8_t r, uint8_t g, uint8_t b)
		{
			if (!light_dll) {
				return result::not_initialized;
			}

			_bstr_t device_bstr{ device };

			const auto result = MLAPI_SetLedColor(device_bstr.GetBSTR(), led_id, r, g, b);
			return result_from_int(result);
		}

		result set_led_speed(const wchar_t* device, uint32_t led_id, uint32_t speed)
		{
			if (!light_dll) {
				return result::not_initialized;
			}

			_bstr_t device_bstr{ device };

			const auto result = MLAPI_SetLedSpeed(device_bstr.GetBSTR(), led_id, speed);
			return result_from_int(result);
		}

		result set_led_brightness(const wchar_t* device, uint32_t led_id, uint32_t brightness)
		{
			if (!light_dll) {
				return result::not_initialized;
			}

			_bstr_t device_bstr{ device };
			const auto result = MLAPI_SetLedBright(device_bstr.GetBSTR(), led_id, brightness);
			return result_from_int(result);
		}
	}
}
