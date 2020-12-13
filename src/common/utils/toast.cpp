#include "toast.hpp"
#include "string.hpp"

#pragma warning(push)
#pragma warning(disable: 6387)
#include <wintoastlib.h>
#pragma warning(pop)

namespace utils::toast
{
	namespace
	{
		bool initialize()
		{
			static bool initialized = false;
			static bool success = false;
			if (initialized)
			{
				return success;
			}

			initialized = true;
			auto* instance = WinToastLib::WinToast::instance();
			if(!instance)
			{
				success = false;
				return success;
			}

			instance->setAppName(L"IW6x");
			instance->setAppUserModelId(
					WinToastLib::WinToast::configureAUMI(L"X Labs", L"iw6x", L"", L"20201212"));

			WinToastLib::WinToast::WinToastError error;
			success = instance->initialize(&error);

			return success;
		}

		class toast_handler : public WinToastLib::IWinToastHandler
		{
		public:
		    void toastActivated() const override {}
		    void toastActivated(const int /*actionIndex*/) const override {}
		    void toastFailed() const override {}
		    void toastDismissed(WinToastDismissalReason /*state*/) const override {}
		};
	}

	bool show(const std::string& title, const std::string& text)
	{
		if(!initialize())
		{
			return false;
		}

		WinToastLib::WinToastTemplate toast_template(WinToastLib::WinToastTemplate::Text02);
		toast_template.setTextField(string::convert(title), WinToastLib::WinToastTemplate::FirstLine);
		toast_template.setTextField(string::convert(text), WinToastLib::WinToastTemplate::SecondLine);
		toast_template.setDuration(WinToastLib::WinToastTemplate::Long);
		toast_template.setAudioPath(WinToastLib::WinToastTemplate::Reminder);

		return SUCCEEDED(WinToastLib::WinToast::instance()->showToast(toast_template, new toast_handler()));
	}

	bool show(const std::string& title, const std::string& text, const std::string& image)
	{
		if(!initialize())
		{
			return false;
		}

		WinToastLib::WinToastTemplate toast_template(WinToastLib::WinToastTemplate::ImageAndText02);
		toast_template.setTextField(string::convert(title), WinToastLib::WinToastTemplate::FirstLine);
		toast_template.setTextField(string::convert(text), WinToastLib::WinToastTemplate::SecondLine);
		toast_template.setDuration(WinToastLib::WinToastTemplate::Long);
		toast_template.setAudioPath(WinToastLib::WinToastTemplate::Reminder);
		toast_template.setImagePath(string::convert(image));

		return SUCCEEDED(WinToastLib::WinToast::instance()->showToast(toast_template, new toast_handler()));
	}
}
