#include <YYToolkit/Shared.hpp>

using namespace Aurie;
using namespace YYTK;

static YYTKInterface* g_ModuleInterface = nullptr;

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

static PFUNC_YYGMLScript original_function = nullptr;
static PFUNC_YYGMLScript original_gui_draw = nullptr;
static FWCodeEvent *event;

void FrameCallback(FWFrame& FrameContext)
{
	UNREFERENCED_PARAMETER(FrameContext);

	static uint32_t frame_counter = 0;
	
	// if (frame_counter % 60 == 0)
	// 	g_ModuleInterface->PrintWarning("[Example Plugin] - 60 frames have passed! Framecount: %u", frame_counter);

	frame_counter++;

}

bool ObjectCallback(CInstance* self, CInstance* other, CCode* code, int count, RValue* args)
{
	// g_ModuleInterface->PrintWarning("[Example Plugin] - ExecuteIt: %s", code->m_Functions->m_Name);
	// const char *name = code->GetName();
	// if (name != nullptr)
	// {
	// 	g_ModuleInterface->PrintWarning("[Example Plugin] - ExecuteIt: %p", name);
	// }
	return true;
}

typedef RValue& (__fastcall *F)(CInstance *Self, CInstance *Other, RValue& ReturnValue, int ArgumentCount, RValue** Arguments);

RValue& Hook(CInstance *Self, CInstance *Other, RValue& ReturnValue, int ArgumentCount, RValue** Arguments)
{
	g_ModuleInterface->PrintWarning("[Example Plugin] - Hook %d %p", ArgumentCount, original_function);

//	g_ModuleInterface->PrintWarning("[Example Plugin] Self: %s", );

	g_ModuleInterface->PrintWarning("[Example Plugin] 0: %d %f", Arguments[0]->m_Kind, Arguments[0]->AsReal()); // 0
	g_ModuleInterface->PrintWarning("[Example Plugin] 1: %d %f", Arguments[1]->m_Kind, Arguments[1]->AsReal()); // 0 
	g_ModuleInterface->PrintWarning("[Example Plugin] 2: %d %d", Arguments[2]->m_Kind, Arguments[2]->AsBool()); // 13
	g_ModuleInterface->PrintWarning("[Example Plugin] 3: %d %f", Arguments[3]->m_Kind, Arguments[3]->AsReal()); // 0 
	g_ModuleInterface->PrintWarning("[Example Plugin] 4: %d", Arguments[4]->m_Kind); // 15
	g_ModuleInterface->PrintWarning("[Example Plugin] 5: %d %f", Arguments[5]->m_Kind, Arguments[5]->AsReal()); // 0

//	return (*reinterpret_cast<F>(*original_function))(Self, Other, ReturnValue, ArgumentCount, Arguments);
	return (*original_function)(Self, Other, ReturnValue, ArgumentCount, Arguments);
}

RValue& GuiDraw(CInstance *Self, CInstance *Other, RValue& ReturnValue, int ArgumentCount, RValue** Arguments)
{
	// g_ModuleInterface->PrintWarning("[Example Plugin] - GuiDraw %d");

	draw_set_font("font_big_num");
	draw_set_color(0xFFFFFF);
	draw_text_transformed(0, 0, "Hello World", 2, 2, 0);

	return (*original_gui_draw)(Self, Other, ReturnValue, ArgumentCount, Arguments);
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
)
{
	UNREFERENCED_PARAMETER(ModulePath);

	AurieStatus last_status = AURIE_SUCCESS;
	
	// Gets a handle to the interface exposed by YYTK
	// You can keep this pointer for future use, as it will not change unless YYTK is unloaded.
	last_status = ObGetInterface(
		"YYTK_Main", 
		(AurieInterfaceBase*&)(g_ModuleInterface)
	);

	// If we can't get the interface, we fail loading.
	if (!AurieSuccess(last_status))
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;

	last_status = g_ModuleInterface->CreateCallback(
		Module,
		EVENT_FRAME,
		FrameCallback,
		0
	);

	if (!AurieSuccess(last_status))
	{
		g_ModuleInterface->Print(CM_LIGHTGREEN, "[Example Plugin] - Failed to register callback!");
	}

// 	last_status = g_ModuleInterface->CreateCallback(
// 	Module,
// 	EVENT_OBJECT_CALL,
// 	ObjectCallback,
// 	0
// );
// 	if (!AurieSuccess(last_status))
// 	{
// 		g_ModuleInterface->Print(CM_LIGHTGREEN, "[Example Plugin] - Failed to register callback!");
// 	}

	CScript *script_data = nullptr;
	last_status = g_ModuleInterface->GetNamedRoutinePointer("gml_Script_floatNumCreate", reinterpret_cast<PVOID*>(&script_data));
	if (!AurieSuccess(last_status))
	{
		g_ModuleInterface->Print(CM_LIGHTGREEN, "[Example Plugin] - Failed to register callback!");
	}
	
	MmCreateHook(
		Module, "my hook", script_data->m_Functions->m_ScriptFunction,
		Hook,
		reinterpret_cast<PVOID*>(&original_function));

	last_status = g_ModuleInterface->GetNamedRoutinePointer("gml_Script_GUIdraw", reinterpret_cast<PVOID*>(&script_data));
	if (!AurieSuccess(last_status))
	{
		g_ModuleInterface->Print(CM_LIGHTGREEN, "[Example Plugin] - Failed to register callback!");
	}
	MmCreateHook(
		Module, "my hook 2", script_data->m_Functions->m_ScriptFunction,
		GuiDraw,
		reinterpret_cast<PVOID*>(&original_gui_draw));

	// CallBuiltin("show_debug_overlay", { true, false });

	g_ModuleInterface->Print(CM_LIGHTGREEN, "[Example Plugin] - Hello from PluginEntry!");
	
	return AURIE_SUCCESS;
}