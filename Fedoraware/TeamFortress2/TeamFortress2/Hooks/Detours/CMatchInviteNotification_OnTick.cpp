#include "../Hooks.h"

MAKE_HOOK(CMatchInviteNotification_OnTick, g_Pattern.Find(L"client.dll", L"55 8B EC 83 EC 0C 56 8B F1 E8 ? ? ? ? 8B 86 ? ? ? ? 85 C0 74 09 83 F8 02 0F 85"), void, __fastcall,
		  void* ecx, void* edx)
{
	static auto CMatchInviteNotification_AcceptMatch = reinterpret_cast<void(__thiscall*)(void*)>(g_Pattern.Find(L"client.dll", L"55 8B EC 83 EC 10 56 8B F1 8B 86 ? ? ? ? 83 E8 00"));
	Hook.Original<FN>()(ecx, edx);

	if (Vars::Misc::InstantAccept.Value)
	{
		CMatchInviteNotification_AcceptMatch(ecx);
	}
}