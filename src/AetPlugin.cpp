#include "AetPlugin.h"
#include "AetImport.h"
#include "AetExport.h"
#include "Misc/StringHelper.h"
#include "FileSystem/FileHelper.h"

namespace AetPlugin
{
	AEGP_PluginID PluginID = -1;
	SPBasicSuite* BasicPicaSuite = nullptr;

	AEGP_Command ExportAetSetCommand = -1;

	namespace
	{
		constexpr std::array AetSetFileExtensions =
		{
			std::array { 'b', 'i', 'n' },
			std::array { 'a', 'e', 'c' },
		};

		A_Err AEGP_FileVerifyCallbackHandler(const A_UTF16Char* filePath, AE_FIM_Refcon refcon, A_Boolean* a_canImport)
		{
			const auto verifyResult = AetImporter::VerifyAetSetImportable(AEUtil::WCast(filePath));
			*a_canImport = (verifyResult == AetImporter::AetSetVerifyResult::Valid);

			return A_Err_NONE;
		}

		A_Err AEGP_FileImportCallbackHandler(const A_UTF16Char* filePath, AE_FIM_ImportOptions importOptions, AE_FIM_SpecialAction action, AEGP_ItemH itemHandle, AE_FIM_Refcon refcon)
		{
			const auto filePathString = std::wstring(AEUtil::WCast(filePath));
			const auto aetSet = AetImporter::LoadAetSet(filePathString);

			if (aetSet == nullptr || aetSet->GetScenes().empty())
				return A_Err_GENERIC;

			A_Err err = A_Err_NONE;

			auto importer = AetImporter(FileSystem::GetDirectory(filePathString));
			ERR(importer.ImportAetSet(*aetSet, importOptions, action, itemHandle));

			return err;
		}

		A_Err RegisterAetSetFileTypeImport(const SuitesData& suites)
		{
			std::array<AEIO_FileKind, 1> fileTypes = {};
			for (auto& fileType : fileTypes)
			{
				fileType.mac.type = 'AETC';
				fileType.mac.creator = AEIO_ANY_CREATOR;
			}

			std::array<AEIO_FileKind, AetSetFileExtensions.size()> fileExtensions = {};
			for (size_t i = 0; i < AetSetFileExtensions.size(); i++)
			{
				fileExtensions[i].ext.pad = '.';
				fileExtensions[i].ext.extension[0] = AetSetFileExtensions[i][0];
				fileExtensions[i].ext.extension[1] = AetSetFileExtensions[i][1];
				fileExtensions[i].ext.extension[2] = AetSetFileExtensions[i][2];
			}

			AE_FIM_ImportFlavorRef importFlavorRef = AE_FIM_ImportFlavorRef_NONE;

			A_Err err = A_Err_NONE;
			ERR(suites.FIMSuite3->AEGP_RegisterImportFlavor("Project DIVA AetSet", &importFlavorRef));
			ERR(suites.FIMSuite3->AEGP_RegisterImportFlavorFileTypes(importFlavorRef,
				static_cast<A_long>(fileTypes.size()), fileTypes.data(),
				static_cast<A_long>(fileExtensions.size()), fileExtensions.data()));

			if (err || importFlavorRef == AE_FIM_ImportFlavorRef_NONE)
				return err;

			AE_FIM_ImportCallbacks importCallbacks = {};
			importCallbacks.refcon = nullptr;
			importCallbacks.import_cb = AEGP_FileImportCallbackHandler;
			importCallbacks.verify_cb = AEGP_FileVerifyCallbackHandler;

			ERR(suites.FIMSuite3->AEGP_RegisterImportFlavorImportCallbacks(importFlavorRef, AE_FIM_ImportFlag_COMP, &importCallbacks));
			return err;
		}

		A_Err AEGP_UpdateMenuHook(AEGP_GlobalRefcon plugin_refconPV, AEGP_UpdateMenuRefcon refconPV, AEGP_WindowType active_window)
		{
			if (!ExportAetSetCommand)
				return A_Err_NONE;

			const SuitesData suites;

			A_Err err = A_Err_NONE;
			ERR(suites.CommandSuite1->AEGP_EnableCommand(ExportAetSetCommand));
			return err;
		}

		A_Err AEGP_CommandHook(AEGP_GlobalRefcon plugin_refconPV, AEGP_CommandRefcon refconPV, AEGP_Command command, AEGP_HookPriority hook_priority, A_Boolean already_handledB, A_Boolean* handledPB)
		{
			*handledPB = false;
			if (command != ExportAetSetCommand)
				return A_Err_NONE;

			const SuitesData suites;

			A_Err err = A_Err_NONE;
			suites.UtilitySuite3->AEGP_ReportInfo(PluginID, __FUNCTION__"(): Ehm yeah, hi I guess?");

			// auto exporter = AetExporter();
			// err = exporter.ExportAetSet();
			*handledPB = true;

			return err;
		}

		A_Err RegisterAetSetFileTypeExport(const SuitesData& suites)
		{
			A_Err err = A_Err_NONE;

			ERR(suites.CommandSuite1->AEGP_GetUniqueCommand(&ExportAetSetCommand));
			ERR(suites.CommandSuite1->AEGP_InsertMenuCommand(ExportAetSetCommand, "Export Project DIVA AetSet", AEGP_Menu_EXPORT, AEGP_MENU_INSERT_SORTED));

			ERR(suites.RegisterSuite5->AEGP_RegisterCommandHook(PluginID, AEGP_HP_BeforeAE, AEGP_Command_ALL, AEGP_CommandHook, nullptr));
			ERR(suites.RegisterSuite5->AEGP_RegisterUpdateMenuHook(PluginID, AEGP_UpdateMenuHook, nullptr));

			return err;
		}

		A_Err RegisterAetSetFileType()
		{
			const SuitesData suites;

			A_Err err = A_Err_NONE;
			ERR(RegisterAetSetFileTypeImport(suites));
			ERR(RegisterAetSetFileTypeExport(suites));
			return err;
		}
	}
}

A_Err EntryPointFunc(SPBasicSuite* pica_basicP, A_long major_versionL, A_long minor_versionL, AEGP_PluginID aegp_plugin_id, AEGP_GlobalRefcon* global_refconP)
{
	AetPlugin::BasicPicaSuite = pica_basicP;
	AetPlugin::PluginID = aegp_plugin_id;

	const auto error = AetPlugin::RegisterAetSetFileType();
	return error;
}