#include <deque>
#include <YYToolkit/Shared.hpp>

using namespace Aurie;
using namespace YYTK;

struct Damage
{
	double value;
	std::string targetName;
};

static YYTKInterface* g_ModuleInterface = nullptr;
static PFUNC_YYGMLScript g_FloatNumCreate = nullptr;
static PFUNC_YYGMLScript g_GuiDraw = nullptr;

static std::vector<Damage> g_CurrentDamages;
static std::deque<Damage> g_DamageHistory;

inline static RValue CallBuiltin(const char* FunctionName, std::vector<RValue> Arguments) {
	return g_ModuleInterface->CallBuiltin(FunctionName, Arguments);
}

inline static double GetAssetIndexFromName(const char* name) {
	double index = CallBuiltin("asset_get_index", { name }).AsReal();
	return index;
}

inline static void draw_set_font(const char* font) {
	CallBuiltin("draw_set_font", { GetAssetIndexFromName(font) });
}

inline static void draw_set_color(double col) {
	CallBuiltin("draw_set_color", { col });
}

inline static void draw_text(double x, double y, const char* string) {
	CallBuiltin("draw_text", { x, y, string });
}

inline static void draw_text_transformed(double x, double y, const char* string, double xscale, double yscale, double angle) {
	CallBuiltin("draw_text_transformed", { x, y, string, xscale, yscale, angle });
}

inline static bool keyboard_check_pressed(double key) {
	return CallBuiltin("keyboard_check_pressed", { key }).AsBool();
}

static std::string formatWithSuffix(double value)
{
	const char* suffixes[] = { "", "k", "M", "B", "T", "Qa" };
	int suffixIndex = 0;

	while (value / 1000 >= 1 && suffixIndex < 5)
	{
		value /= 1000;
		suffixIndex++;
	}

	return std::format("{:.1f}{}", value, suffixes[suffixIndex]);
}

// 1=damage: double
// 2=isCrit: bool
// 4=target object: pointer
RValue& FloatNumCreateHook(CInstance *Self, CInstance *Other, RValue& ReturnValue, int ArgumentCount, RValue** Arguments)
{
	// g_ModuleInterface->PrintWarning("[ChroniconDPS] FloatNumCreateHook: self=%s, other=%s, 0=0x%x, damage=%f, isCrit=%d, 3=0x%x, target=%d, 5=0x%x",
	// 	Self->m_Object->m_Name, Other->m_Object->m_Name,
	// 	Arguments[0]->AsReal(), Arguments[1]->AsReal(), Arguments[2]->AsBool(), Arguments[3]->AsReal(), Arguments[4]->m_i32, Arguments[5]->AsReal());

	// CInstance* target;
	// if (!AurieSuccess(g_ModuleInterface->GetInstanceObject(Arguments[4]->m_i32, target)))
	// {
	// 	g_ModuleInterface->PrintError(__FILE__, __LINE__, "[ChroniconDPS] Failed to get instance object");
	// }
	// if (target)
	// {
	// 	RValue inst = new RValue(target);
	// 	std::string out;
	// 	g_ModuleInterface->RValueToString(inst, out);
	// 	g_ModuleInterface->PrintWarning(out);
	// 	g_ModuleInterface->PrintWarning(target->m_Object->m_Name);
	//
	// 	// int count = g_ModuleInterface->GetRunnerInterface().StructGetKeys(&inst, nullptr, nullptr);
	// 	// g_ModuleInterface->PrintWarning("count: %d", count);
	//
	// 	if (!AurieSuccess(g_ModuleInterface->EnumInstanceMembers(Other, [](const char* name, RValue* value) {
	// 		std::string out;
	// 		g_ModuleInterface->RValueToString(*value, out);
	// 		g_ModuleInterface->PrintWarning("name: %s, value: %s", name, out);
	// 		return false;
	// 	})))
	// 	{
	// 		g_ModuleInterface->PrintError(__FILE__, __LINE__, "[ChroniconDPS] Failed to enum instance members");
	// 	}
	// }

	// g_ModuleInterface->PrintWarning("[Example Plugin] 0: %d %f", Arguments[0]->m_Kind, Arguments[0]->AsReal()); // 0
	// g_ModuleInterface->PrintWarning("[Example Plugin] 1: %d %f", Arguments[1]->m_Kind, Arguments[1]->AsReal()); // 0 
	// g_ModuleInterface->PrintWarning("[Example Plugin] 2: %d %d", Arguments[2]->m_Kind, Arguments[2]->AsBool()); // 13
	// g_ModuleInterface->PrintWarning("[Example Plugin] 3: %d %f", Arguments[3]->m_Kind, Arguments[3]->AsReal()); // 0 
	// g_ModuleInterface->PrintWarning("[Example Plugin] 4: %d", Arguments[4]->m_Kind); // 15
	// g_ModuleInterface->PrintWarning("[Example Plugin] 5: %d %f", Arguments[5]->m_Kind, Arguments[5]->AsReal()); // 0


	// 0: Physical
	// 1: Fire
	// 3: Lightning
	// 5: Holy
	RValue* element;
	g_ModuleInterface->GetInstanceMember(Other, "element", element);
	// g_ModuleInterface->PrintWarning("%f", element->AsReal());

	auto targetName = std::string(Self->m_Object->m_Name);
	if (targetName != "obj_player")
	{
		g_CurrentDamages.push_back({ Arguments[1]->AsReal(), targetName });
	}

	return (*g_FloatNumCreate)(Self, Other, ReturnValue, ArgumentCount, Arguments);
}

RValue& GuiDrawHook(CInstance *Self, CInstance *Other, RValue& ReturnValue, int ArgumentCount, RValue** Arguments)
{
    // g_ModuleInterface->PrintWarning("[ChroniconDPS] - GuiDrawHook %d %p", ArgumentCount, g_FloatNumCreate);
	static long frameCount = 0;
	static std::string dpsText;
	static bool isOneShot = false;
	static double peakDPS = 0.0;

	double damage = 0.0;
	for (auto d : g_CurrentDamages)
	{
		damage += d.value;
	}
	g_CurrentDamages.clear();

	if (isOneShot)
	{
		if (!(damage == 0.0 && g_DamageHistory.empty()) && g_DamageHistory.size() <= 5 * 60)
		{
			g_DamageHistory.push_back({ damage, "" });
		}
	} else
	{
		g_DamageHistory.push_back({ damage, "" });
		if (g_DamageHistory.size() > 5 * 60 && !isOneShot)
		{
			g_DamageHistory.pop_front();
		}
	}

	double total = 0.0;
	for (auto d : g_DamageHistory)
	{
		total += d.value;
	}

	double dps = floor(total / 5);
	if (dps > peakDPS)
	{
		peakDPS = dps;
	}

	if (frameCount % 30 == 0)
	{
		dpsText = std::format("DPS: {}", formatWithSuffix(dps));
	}
	frameCount++;

	if (keyboard_check_pressed(0x70)) // VK_F1
	{
		g_DamageHistory.clear();
		peakDPS = 0.0;
		isOneShot = false;
	}
	if (keyboard_check_pressed(0x71)) // VK_F2
	{
		g_DamageHistory.clear();
		peakDPS = 0.0;
		isOneShot = true;
	}

	draw_set_font("font_large_GUI_latin");
	if (isOneShot)
	{
		draw_set_color(0x00FFFF);
	}
	else
	{
		draw_set_color(0xFFFFFF);
	}
	const auto text = std::format("{}\nPeak DPS: {}", dpsText, formatWithSuffix(peakDPS));
	draw_text_transformed(0, 0,  text.c_str(), 2, 2, 0);

	return (*g_GuiDraw)(Self, Other, ReturnValue, ArgumentCount, Arguments);
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	UNREFERENCED_PARAMETER(ModulePath);

	AurieStatus last_status = AURIE_SUCCESS;
	
	if (!AurieSuccess(ObGetInterface("YYTK_Main", (AurieInterfaceBase*&)(g_ModuleInterface))))
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;

	CScript *script_data = nullptr;

	if (!AurieSuccess(g_ModuleInterface->GetNamedRoutinePointer("gml_Script_floatNumCreate", reinterpret_cast<PVOID*>(&script_data)))) 
		g_ModuleInterface->PrintError(__FILE__, __LINE__, "[ChroniconDPS] Failed to get pointer for gml_Script_floatNumCreate");
	if (!AurieSuccess(MmCreateHook(
		Module, "gml_Script_floatNumCreate",
		script_data->m_Functions->m_ScriptFunction,
		FloatNumCreateHook,
		reinterpret_cast<PVOID*>(&g_FloatNumCreate))))
		g_ModuleInterface->PrintError(__FILE__, __LINE__, "[ChroniconDPS] Failed to register hook for gml_Script_floatNumCreate");

	if (!AurieSuccess(g_ModuleInterface->GetNamedRoutinePointer("gml_Script_GUIdraw", reinterpret_cast<PVOID*>(&script_data)))) 
		g_ModuleInterface->PrintError(__FILE__, __LINE__, "[ChroniconDPS] Failed to get pointer for gml_Script_GUIdraw");
	if (!AurieSuccess(MmCreateHook(
		Module, "gml_Script_GUIdraw",
		script_data->m_Functions->m_ScriptFunction,
		GuiDrawHook,
		reinterpret_cast<PVOID*>(&g_GuiDraw))))
		g_ModuleInterface->PrintError(__FILE__, __LINE__, "[ChroniconDPS] Failed to register hook for gml_GUIdraw");

	g_ModuleInterface->Print(CM_LIGHTGREEN, "[ChroniconDPS] Loaded");
	
	return AURIE_SUCCESS;
}