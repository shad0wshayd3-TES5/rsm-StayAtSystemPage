namespace
{
	class JournalMenuEx :
		public RE::JournalMenu
	{
	public:
		enum class Tab : std::uint32_t
		{
			kQuest,
			kStats,
			kSystem
		};

		void Hook_Accept(RE::FxDelegateHandler::CallbackProcessor* a_cbProc)
		{
			_Accept(this, a_cbProc);
			fxDelegate->callbacks.Remove("RememberCurrentTabIndex");
			a_cbProc->Process("RememberCurrentTabIndex", RememberCurrentTabIndex);
		}

		RE::UI_MESSAGE_RESULTS Hook_ProcessMessage(RE::UIMessage& a_message)
		{
			switch (a_message.type.get()) {
			case RE::UI_MESSAGE_TYPE::kShow:
				{
					auto UI = RE::UI::GetSingleton();
					auto InterfaceStrings = RE::InterfaceStrings::GetSingleton();
					if (UI && InterfaceStrings && UI->IsMenuOpen(InterfaceStrings->mapMenu)) {
						*_savedTabIdx = Tab::kQuest;
					} else {
						*_savedTabIdx = Tab::kSystem;
					}
				}

			default:
				break;
			}

			return _ProcessMessage(this, a_message);
		}

		static void RememberCurrentTabIndex([[maybe_unused]] const RE::FxDelegateArgs& a_params)
		{
			return;
		}

		static void InstallHooks()
		{
			REL::Relocation<std::uintptr_t> vtbl{ RE::Offset::JournalMenu::Vtbl };
			_Accept = vtbl.write_vfunc(0x01, &JournalMenuEx::Hook_Accept);
			_ProcessMessage = vtbl.write_vfunc(0x04, &JournalMenuEx::Hook_ProcessMessage);
		}

		static inline REL::Relocation<decltype(&Hook_Accept)> _Accept;
		static inline REL::Relocation<decltype(&Hook_ProcessMessage)> _ProcessMessage;
		static inline REL::Relocation<Tab*> _savedTabIdx{ REL::ID(406697) };
	};

	void InitializeLog()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			stl::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log", Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
		const auto level = spdlog::level::trace;
#else
		const auto level = spdlog::level::info;
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%^%l%$] %v"s);
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);

	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	SKSE::Init(a_skse);

	JournalMenuEx::InstallHooks();

	return true;
}
