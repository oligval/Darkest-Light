#include <Windows.h>
#include <string>
#include <mlapi_wrapper.h>
#include <optional>
#include <exception>

union led_color
{
	struct
	{
		uint8_t r, g, b;
	};
	DWORD color;
};

static_assert(sizeof(led_color) == 4);

struct led_info
{
	std::wstring style;
	led_color color;
	DWORD brightness, speed;
};

struct device_info
{
	std::wstring name;
	std::vector<led_info> led_info;
};

namespace ML = starl1ght::mystic_light;

std::vector<device_info> get_led_devices()
{
	auto [result, devices, leds] = ML::get_device_info();
	if (result != ML::result::ok || devices.empty() || leds.empty() || leds.size() != devices.size()) {
		throw std::exception{ "Unable to get devices information" };
	}

	std::vector<device_info> dev;

	for (ULONG i = 0; i < devices.size(); ++i) {
		std::vector<led_info> led_vec;

		for (ULONG led = 0; led < leds[i]; ++led) {
			const auto& [r_st, style] = ML::get_led_style(devices[i].c_str(), led);
			if (r_st != ML::result::ok) {
				throw std::exception{ "Unable to get style info" };
			}

			const auto& [r_cl, r, g, b] = ML::get_led_color(devices[i].c_str(), led);
			if (r_cl != ML::result::ok) {
				throw std::exception{ "Unable to get color info" };
			}

			const auto& [r_br, bright] = ML::get_led_bright(devices[i].c_str(), led);
			if (r_br != ML::result::ok) {
				throw std::exception{ "Unable to get brightness info" };
			}

			const auto& [r_sp, speed] = ML::get_led_speed(devices[i].c_str(), led);
			if (r_sp != ML::result::ok) {
				throw std::exception{ "Unable to get speed info" };
			}

			led_color lc{ r, g ,b };
			led_vec.push_back(led_info{ style, lc, bright,speed });
		}

		device_info dev_tmp{ devices[i], std::move(led_vec) };
		dev.push_back(std::move(dev_tmp));
	}

	return dev;
}


bool is_everything_off(const std::vector<device_info>& devices)
{
	if (devices.empty()) {
		throw std::exception{ "Empty devices on is_everything_off" };
	}

	for (const auto& device : devices) {
		for (const auto& led : device.led_info) {
			if (led.style != L"Off" && led.style != L"off") {
				return false;
			}
		}
	}

	return true;
}

void darkest_light(const std::vector<device_info>& devices)
{
	if (devices.empty()) {
		throw std::exception{ "Empty devices on darkest_light" };
	}

	for (const auto& device : devices) {
		for (ULONG i = 0; i < device.led_info.size(); ++i) {
			if (ML::set_led_style(device.name.c_str(), i, L"Off") != ML::result::ok) {
				throw std::exception{ "Unable to set 'Off' to some light" };
			}
		}
	}
}

void mystic_light(const std::vector<device_info>& devices)
{
	if (devices.empty()) {
		throw std::exception{ "Empty devices on mystic_light" };
	}

	for (const auto& device : devices) {
		for (ULONG i = 0; i < device.led_info.size(); ++i) {

			if (ML::set_led_brightness(device.name.c_str(), i, device.led_info[i].brightness) != ML::result::ok) {
				throw std::exception{ "Unable to set brightness to some light" };
			}

			if (ML::set_led_speed(device.name.c_str(), i, device.led_info[i].speed) != ML::result::ok) {
				throw std::exception{ "Unable to set speed to some light" };
			}

			const auto r = device.led_info[i].color.r;
			const auto g = device.led_info[i].color.g;
			const auto b = device.led_info[i].color.b;

			if (ML::set_led_style(device.name.c_str(), i, device.led_info[i].style.c_str()) != ML::result::ok) {
				throw std::exception{ "Unable to set style to some light" };
			}

			const auto color_result = ML::set_led_color(device.name.c_str(), i, r, g, b);
			if (color_result != ML::result::ok && color_result != ML::result::not_supported) {
				throw std::exception{ "Unable to set color to some light" };
			}
		}
	}
}

std::vector<device_info> read_devices()
{
	HKEY hk;
	RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Starl1ght\\Darkest-Light", 0, 0, 0, KEY_ALL_ACCESS, 0, &hk, NULL);

	wchar_t bufford[1488] = {};
	DWORD tp, sz = 1488 * sizeof(wchar_t);
	RegQueryValueExW(hk, NULL, 0, &tp, (BYTE*)bufford, &sz);

	std::vector<device_info> dev;

	const auto extract = [hk](const std::wstring& device_name)
	{
		DWORD cnt, sz = 4;
		RegQueryValueExW(hk, (device_name + L"-count").c_str(), 0, NULL, (BYTE*)&cnt, &sz);

		std::vector<led_info> li;

		for (size_t i = 0; i < cnt; ++i) {
			led_info lop;

			const auto& base_val = device_name + L"-" + std::to_wstring(i) + L"-";

			RegQueryValueExW(hk, (base_val + L"color").c_str(), 0, 0, (BYTE*)&lop.color, &sz);
			RegQueryValueExW(hk, (base_val + L"brightness").c_str(), 0, 0, (BYTE*)&lop.brightness, &sz);
			RegQueryValueExW(hk, (base_val + L"speed").c_str(), 0, 0, (BYTE*)&lop.speed, &sz);

			sz = 14214;
			wchar_t bufo[1411] = {};
			RegQueryValueExW(hk, (base_val + L"style").c_str(), 0, 0, (BYTE*)bufo, &sz);
			lop.style = bufo;

			li.push_back(std::move(lop));
		}
		return device_info{ device_name, std::move(li) };
	};


	size_t bg = 0;
	for (size_t i = 0; i < 1488; ++i) {
		if (bufford[i] == 0) {
			
	

			const std::wstring dvnm(bufford + bg, bufford + i);
			dev.push_back(extract(dvnm));
			bg = i + 1;

			if (bufford[i + 1] == 0) {
				break;
			}
		}

	}




	RegCloseKey(hk);
	return dev;
}

template <typename T>
void save_to_reg(HKEY hk, const std::wstring& valname, const T& val)
{
	if constexpr (std::is_same_v<std::wstring, T>) {
		if (RegSetValueExW(hk, valname.c_str(), 0, REG_SZ, (const BYTE*)val.c_str(), val.size() * sizeof(wchar_t)) != ERROR_SUCCESS) {
			throw std::exception{ "Cannot save style" };
		}
		return;
	}

	if constexpr (std::is_same_v<DWORD, T>) {
		if (RegSetValueExW(hk, valname.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) != ERROR_SUCCESS) {
			throw std::exception{ "Cannot save style" };
		}
		return;
	}
	throw std::exception{ "What just happened?" };
}


void save_devices(const std::vector<device_info>& devices)
{
	if (devices.empty()) {
		throw std::exception{ "Empty devices on save_devices" };
	}

	RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Starl1ght\\Darkest-Light");

	HKEY hk;
	RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Starl1ght\\Darkest-Light", 0, 0, 0, KEY_ALL_ACCESS, 0, &hk, NULL);

	std::wstring device_multi_sz;
	for (const auto& device : devices) {
		device_multi_sz.append(device.name);
		device_multi_sz.push_back(0);

		save_to_reg(hk, device.name + L"-count", DWORD{ device.led_info.size() });

		for (size_t i = 0; i < device.led_info.size(); ++i) {
			const auto& base_val = device.name + L"-" + std::to_wstring(i) + L"-";
			save_to_reg(hk, base_val + L"style", device.led_info[i].style);
			save_to_reg(hk, base_val + L"color", device.led_info[i].color.color);
			save_to_reg(hk, base_val + L"speed", device.led_info[i].speed);
			save_to_reg(hk, base_val + L"brightness", device.led_info[i].brightness);
		}
	}
	device_multi_sz.push_back(0); // 2nd end for REG_MULTI_SZ;

	RegSetValueExW(hk, nullptr, 0, REG_MULTI_SZ, (const BYTE*)device_multi_sz.c_str(), device_multi_sz.size() * sizeof(wchar_t));
	RegCloseKey(hk);
}

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ML::initialize(L"MysticLight_SDK.dll");

	if (ML::initialize(L"MysticLight_SDK.dll") != ML::result::ok) {
		MessageBoxA(NULL, "Unable to load MysticLight_SDK.dll", "Err0r", MB_OK);
		return -1;
	}

	try {
		const auto& devices = get_led_devices();

		if (is_everything_off(devices)) {
			const std::vector<device_info>& new_devices = read_devices();
			mystic_light(new_devices);
		}
		else {
			save_devices(devices);
			darkest_light(devices);
		}
	}
	catch (const std::exception& e) {
		MessageBoxA(NULL, e.what(), "Exceptionado", MB_OK);
		return -1;
	}
	return 0;

	/*
	HKEY hk;
	RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Starl1ght\\Darkest-Light", 0, 0, 0, KEY_ALL_ACCESS, 0, &hk, NULL);

	const auto sz = devices[0].size() + 1 + 1;
	wchar_t* bub = new wchar_t[sz];
	RtlZeroMemory(bub, sz * 2);
	memcpy_s(bub, sz * 2, devices[0].c_str(), devices[0].size() * 2);

	RegSetKeyValueW(hk, nullptr, nullptr, REG_MULTI_SZ, bub, sz * 2);

	for (ULONG i = 0; i < inf.size(); ++i)
	{
		const auto base_st = devices[0] + L'_' + std::to_wstring(i) + L'_';

		const auto style_v = base_st + L"style";
		RegSetValueExW(hk, style_v.c_str(), 0, REG_SZ, (const BYTE*)inf[i].style.c_str(), inf[i].style.size() * 2 + 2);

		const auto r_v = base_st + L"red";
		RegSetValueExW(hk, r_v.c_str(), 0, REG_DWORD, (const BYTE*)&inf[i].red, 4);
		const auto r_g = base_st + L"green";
		RegSetValueExW(hk, r_g.c_str(), 0, REG_DWORD, (const BYTE*)&inf[i].green, 4);
		const auto r_b = base_st + L"blue";
		RegSetValueExW(hk, r_b.c_str(), 0, REG_DWORD, (const BYTE*)&inf[i].blue, 4);
		const auto r_sp = base_st + L"speed";
		RegSetValueExW(hk, r_sp.c_str(), 0, REG_DWORD, (const BYTE*)&inf[i].speed, 4);
		const auto r_br = base_st + L"brightness";
		RegSetValueExW(hk, r_br.c_str(), 0, REG_DWORD, (const BYTE*)&inf[i].brightness, 4);
	}
	RegCloseKey(hk);

	wchar_t buf[2048];
	RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Starl1ght\\Darkest-Light", 0, 0, 0, KEY_ALL_ACCESS, 0, &hk, NULL);
	DWORD a, blue, c, d;
	RegQueryValueExW(hk, nullptr, 0, &a, (BYTE*)buf, &blue);

	/*ML::set_led_style(L"MSI_MB", 0, L"Off");
	ML::set_led_style(L"MSI_MB", 1, L"Off");
	ML::set_led_style(L"MSI_MB", 2, L"Off");
	ML::set_led_style(L"MSI_MB", 3, L"Off");
	ML::set_led_style(L"MSI_MB", 4, L"Off");
	ML::set_led_style(L"MSI_MB", 5, L"Off");
	ML::set_led_style(L"MSI_MB", 6, L"Off");
	ML::set_led_style(L"MSI_MB", 7, L"Off");*/
}