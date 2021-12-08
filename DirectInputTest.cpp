#pragma comment(lib, "dinput.lib")
#pragma comment(lib, "dxguid.lib")

#define DIRECTINPUT_VERSION 0x0500

#include <Windows.h>
#include <dinput.h>
#include <wrl/client.h>

#include <charconv>
#include <cstdlib>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace std::string_view_literals;

static ComPtr<IDirectInput> dinput;
static std::vector<DIDEVICEINSTANCE> devices;

int main(int, char **);
static void main2(int, char **);
static BOOL CALLBACK EnumCB(LPCDIDEVICEINSTANCE, LPVOID);
static void Acquire(const GUID &, LPCDIDATAFORMAT, DWORD);
static std::string Guid2String(const GUID &);

template<class T>
static auto as_unsigned(const T &value)
{
	return static_cast<std::make_unsigned_t<T>>(value);
}

int main(int argc, char **argv)
{
	try
	{
		main2(argc, argv);
		return EXIT_SUCCESS;
	}
	catch (const std::runtime_error &e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}

static void main2(int argc, char **argv)
{
	DWORD cooperativeLevel{DISCL_EXCLUSIVE | DISCL_FOREGROUND};

	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] == "-cl"sv && i < argc - 1)
		{
			++i;
			if (std::from_chars(argv[i], argv[i] + std::char_traits<char>::length(argv[i]), cooperativeLevel, 16).ec != std::errc{})
				throw std::runtime_error{std::format("Cannot parse \"{}\"", argv[i])};
		}
		else
		{
			throw std::runtime_error{std::format("Invalid command line argument: \"{}\"", argv[i])};
		}
	}

	std::cout << std::format("Cooperative level: 0x{:X}\n\n", cooperativeLevel);

	const auto hrCreate = DirectInputCreate(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, &dinput, nullptr);
	if (hrCreate != DI_OK)
		throw std::runtime_error{std::format("DirectInputCreate returned {:X}", as_unsigned(hrCreate))};

	const auto hrEnum = dinput->EnumDevices(DIDEVTYPEJOYSTICK_GAMEPAD, EnumCB, nullptr, DIEDFL_ATTACHEDONLY);
	if (hrEnum != DI_OK)
		throw std::runtime_error{std::format("EnumDevices returned {:X}", as_unsigned(hrEnum))};

	std::cout << devices.size() << " controllers enumerated\n\n";

	std::size_t i{0};
	for (const auto &devInfo : devices)
	{
		std::cout <<
			"Device #" << i << "\n"
			" guidInstance = " << Guid2String(devInfo.guidInstance) << "\n"
			" guidProduct = " << Guid2String(devInfo.guidProduct) << "\n"
			" dwDevType = " << std::format("0x{:X}", devInfo.dwDevType) << "\n"
			" tszInstanceName = \"" << devInfo.tszInstanceName << "\"\n"
			" tszProductName = \"" << devInfo.tszProductName << "\"\n"
			" guidFFDriver = " << Guid2String(devInfo.guidFFDriver) << "\n"
			" wUsagePage = " << std::format("0x{:X}", devInfo.wUsagePage) << "\n"
			" wUsage = " << std::format("0x{:X}", devInfo.wUsage) << "\n\n";

		constexpr std::pair<std::string_view, LPCDIDATAFORMAT> dataFormats[]
		{
			{ "c_dfDIJoystick" , &c_dfDIJoystick  },
			{ "c_dfDIJoystick2", &c_dfDIJoystick2 }
		};
		for (const auto &df : dataFormats)
		{
			std::cout << std::format("Acquire(df={})\n\n", df.first);
			try
			{
				Acquire(devInfo.guidInstance, df.second, cooperativeLevel);
			}
			catch (const std::runtime_error &e)
			{
				std::cerr << " " << e.what() << "\n\n";
			}
		}

		++i;
	}
}

static BOOL CALLBACK EnumCB(const LPCDIDEVICEINSTANCE lpddi, LPVOID)
{
	devices.push_back(*lpddi);
	return DIENUM_CONTINUE;
}

static void Acquire(const GUID &guid, const LPCDIDATAFORMAT dataFormat, const DWORD cooperativeLevel)
{
	ComPtr<IDirectInputDevice> dev;
	const auto hrCreate = dinput->CreateDevice(guid, &dev, nullptr);
	if (hrCreate != DI_OK)
		throw std::runtime_error{std::format("CreateDevice returned {:X}", as_unsigned(hrCreate))};

	ComPtr<IDirectInputDevice2> dev2;
	const auto hrQueryIfc = dev->QueryInterface(IID_IDirectInputDevice2, &dev2);
	if (hrQueryIfc != S_OK)
		throw std::runtime_error{std::format("QueryInterface (IDirectInputDevice2) returned {:X}", as_unsigned(hrQueryIfc))};

	const auto hrSetFormat = dev2->SetDataFormat(dataFormat);
	if (hrSetFormat != S_OK)
		throw std::runtime_error{std::format("SetDataFormat returned {:X}", as_unsigned(hrSetFormat))};

	const auto hrSetCL = dev2->SetCooperativeLevel(GetConsoleWindow(), cooperativeLevel);
	if (hrSetCL != S_OK)
		throw std::runtime_error{std::format("SetCooperativeLevel returned {:X}", as_unsigned(hrSetCL))};

	const auto hrAcquire = dev2->Acquire();
	if (hrAcquire != S_OK)
		throw std::runtime_error{std::format("Acquire returned {:X}", as_unsigned(hrAcquire))};

	const auto hrUnacquire = dev2->Unacquire();
	if (hrUnacquire != S_OK)
		throw std::runtime_error{std::format("Unacquire returned {:X}", as_unsigned(hrUnacquire))};
}

static std::string Guid2String(const GUID &guid)
{
	return std::format(
		"{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]
	);
}
