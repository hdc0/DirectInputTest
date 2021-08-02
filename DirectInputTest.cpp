#pragma comment(lib, "dinput.lib")
#pragma comment(lib, "dxguid.lib")

#define DIRECTINPUT_VERSION 0x0500

#include <Windows.h>
#include <dinput.h>
#include <wrl/client.h>

#include <cstdlib>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using Microsoft::WRL::ComPtr;

static ComPtr<IDirectInput> dinput;
static std::vector<DIDEVICEINSTANCE> devices;

int main(void);
static void main2(void);
static BOOL CALLBACK EnumCB(LPCDIDEVICEINSTANCE, LPVOID);
static void Acquire(const GUID &);
static std::string Guid2String(const GUID &);

template<class T>
static auto as_unsigned(const T &value)
{
	return static_cast<std::make_unsigned_t<T>>(value);
}

int main(void)
{
	try
	{
		main2();
		return EXIT_SUCCESS;
	}
	catch (const std::runtime_error &e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}

static void main2(void)
{
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

		try
		{
			Acquire(devInfo.guidInstance);
		}
		catch (const std::runtime_error &e)
		{
			std::cerr << " " << e.what() << "\n\n";
		}

		++i;
	}
}

static BOOL CALLBACK EnumCB(const LPCDIDEVICEINSTANCE lpddi, LPVOID)
{
	devices.push_back(*lpddi);
	return DIENUM_CONTINUE;
}

static void Acquire(const GUID &guid)
{
	ComPtr<IDirectInputDevice> dev;
	const auto hrCreate = dinput->CreateDevice(guid, &dev, nullptr);
	if (hrCreate != DI_OK)
		throw std::runtime_error{std::format("CreateDevice returned {:X}", as_unsigned(hrCreate))};

	ComPtr<IDirectInputDevice2> dev2;
	const auto hrQueryIfc = dev->QueryInterface(IID_IDirectInputDevice2, &dev2);
	if (hrQueryIfc != S_OK)
		throw std::runtime_error{std::format("QueryInterface (IDirectInputDevice2) returned {:X}", as_unsigned(hrQueryIfc))};

	const auto hrSetFormat = dev2->SetDataFormat(&c_dfDIJoystick);
	if (hrSetFormat != S_OK)
		throw std::runtime_error{std::format("SetDataFormat returned {:X}", as_unsigned(hrSetFormat))};

	const auto hrSetCL = dev2->SetCooperativeLevel(GetConsoleWindow(), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
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
