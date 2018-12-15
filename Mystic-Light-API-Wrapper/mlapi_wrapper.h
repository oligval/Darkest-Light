#pragma once
#include <tuple>
#include <vector>

namespace starl1ght
{
	namespace mystic_light
	{
		enum class result
		{
			ok,
			generic_error,
			timeout,
			not_implemented,
			not_initialized,
			invalid_argument,
			device_not_found,
			not_supported,
			admin_rights_are_needed,
			not_in_enum
		};

		result initialize(const wchar_t* path_to_dll);

		std::tuple<result, std::vector<std::wstring>, std::vector<uint32_t>> get_device_info();
		std::tuple<result, uint8_t, uint8_t, uint8_t> get_led_color(const wchar_t* device, uint32_t led_id);
		std::tuple<result, std::wstring> get_led_style(const wchar_t* device, uint32_t led_id);
		std::tuple<result, uint32_t> get_led_bright(const wchar_t* device, uint32_t led_id);
		std::tuple<result, uint32_t> get_led_max_bright(const wchar_t* device, uint32_t led_id);
		std::tuple<result, uint32_t> get_led_speed(const wchar_t* device, uint32_t led_id);
		std::tuple<result, uint32_t> get_led_max_speed(const wchar_t* device, uint32_t led_id);
		std::tuple<result> get_led_info(const wchar_t* device, uint32_t led_id);

		result set_led_style(const wchar_t* device, uint32_t led_id, const wchar_t* style);
		result set_led_color(const wchar_t* device, uint32_t led_id, uint8_t r, uint8_t g, uint8_t b);
		result set_led_speed(const wchar_t* device, uint32_t led_id, uint32_t speed);
		result set_led_brightness(const wchar_t* device, uint32_t led_id, uint32_t brightness);
	}
}
