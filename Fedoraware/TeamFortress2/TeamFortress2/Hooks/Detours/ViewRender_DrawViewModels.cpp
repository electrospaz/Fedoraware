#include "../Hooks.h"

MAKE_HOOK(ViewRender_DrawViewModels, g_Pattern.Find(L"client.dll", L"55 8B EC 81 EC ? ? ? ? 8B 15 ? ? ? ? 53 8B"), void, __fastcall,
		  void* ecx, void* edx, const CViewSetup& pViewSetup, bool bDrawViewmodels)
{
	
	static IMaterial*	pGlowColour = nullptr;
	static ITexture*	pRtFullFrame = nullptr;
	static ITexture*	pRenderBuffer1 = nullptr;
	static ITexture*	pRenderBuffer2 = nullptr;
	static IMaterial*	pBlurX = nullptr;
	static IMaterial*	pBlurY = nullptr;
	static IMaterial*	pHaloAddToScreen = nullptr;
	
	if (!pGlowColour)
	{
		pGlowColour = I::MatSystem->Find("dev/glow_color", "Other textures");
	}

	if (!pRtFullFrame)
	{
		pRtFullFrame = I::MatSystem->FindTexture("_rt_FullFrameFB", "RenderTargets");
	}

	if (!pRenderBuffer1)
	{
		pRenderBuffer1 = I::MatSystem->CreateNamedRenderTargetTextureEx("glow_buffer_1", pRtFullFrame->GetActualWidth(), pRtFullFrame->GetActualHeight(),
																		   RT_SIZE_LITERAL, IMAGE_FORMAT_RGB888, MATERIAL_RT_DEPTH_SHARED,
																		   TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_EIGHTBITALPHA, CREATERENDERTARGETFLAGS_HDR);
		pRenderBuffer1->IncrementReferenceCount();
	}

	if (!pRenderBuffer2)
	{
		pRenderBuffer2 = I::MatSystem->CreateNamedRenderTargetTextureEx("glow_buffer_2", pRtFullFrame->GetActualWidth(), pRtFullFrame->GetActualHeight(),
																		   RT_SIZE_LITERAL, IMAGE_FORMAT_RGB888, MATERIAL_RT_DEPTH_SHARED,
																		   TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_EIGHTBITALPHA, CREATERENDERTARGETFLAGS_HDR);
		pRenderBuffer2->IncrementReferenceCount();
	}

	if (!pBlurX)
	{
		static KeyValues* kv = nullptr;
		if (!kv)
		{
			kv = new KeyValues("BlurFilterX");
			kv->SetString("$basetexture", "glow_buffer_1");
		}
		pBlurX = I::MatSystem->Create("pBlurX", kv);
		pBlurX->IncrementReferenceCount();
	}

	if (!pBlurY)
	{
		static KeyValues* kv = nullptr;
		if (!kv)
		{
			kv = new KeyValues("BlurFilterY");
			kv->SetString("$basetexture", "glow_buffer_2");
			kv->SetString("$bloomamount", "10");
		}
		pBlurY = I::MatSystem->Create("pBlurY", kv);
		pBlurY->IncrementReferenceCount();
	}

	if (!pHaloAddToScreen)
	{
		static KeyValues* kv = nullptr;
		if (!kv)
		{
			kv = new KeyValues("UnlitGeneric");
			kv->SetString("$basetexture", "glow_buffer_1");
			kv->SetString("$additive", "1");
		}
		pHaloAddToScreen = I::MatSystem->Create("pHaloAddToScreen", kv);
		pHaloAddToScreen->IncrementReferenceCount();
	}

	if (IMatRenderContext* pContext = I::MatSystem->GetRenderContext())
	{
		g_GlobalInfo.m_bGlowingViewmodel = true;
		ShaderStencilState_t StencilStateDisable = {};
		StencilStateDisable.m_bEnable = false;

		float flOriginalColor[3] = {};
		I::RenderView->GetColorModulation(flOriginalColor);
		float flOriginalBlend = I::RenderView->GetBlend();

		{
			ShaderStencilState_t StencilState = {};
			StencilState.m_bEnable = true;
			StencilState.m_nReferenceValue = 1;
			StencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
			StencilState.m_PassOp = STENCILOPERATION_REPLACE;
			StencilState.m_FailOp = STENCILOPERATION_KEEP;
			StencilState.m_ZFailOp = STENCILOPERATION_REPLACE;
			StencilState.SetStencilState(pContext);
		}
		I::RenderView->SetBlend(1.0f);
		I::RenderView->SetColorModulation(1.0f, 0.f, 1.f);

		Hook.Original<FN>()(ecx, edx, pViewSetup, bDrawViewmodels);

		StencilStateDisable.SetStencilState(pContext);
		I::RenderView->SetColorModulation(1.0f, 0.f, 1.f);
		
		if (IMaterialVar* pColor2Var = pGlowColour->FindVar("$color2", nullptr))
		{
			auto RainbowColour = Utils::Rainbow();
			pColor2Var->SetVecValue(Color::TOFLOAT(RainbowColour.r), Color::TOFLOAT(RainbowColour.g), Color::TOFLOAT(RainbowColour.b));
		}

		I::ModelRender->ForcedMaterialOverride(pGlowColour);
		pContext->PushRenderTargetAndViewport();
		pContext->SetRenderTarget(pRenderBuffer1);
		pContext->Viewport(0, 0, g_ScreenSize.w, g_ScreenSize.h);
		pContext->ClearColor4ub(0, 0, 0, 0);
		pContext->ClearBuffers(true, false, false);
		I::RenderView->SetBlend(1.0f);
		I::RenderView->SetColorModulation(1.0f, 0.f, 1.0f);
		
		Hook.Original<FN>()(ecx, edx, pViewSetup, bDrawViewmodels);

		if (IMaterialVar* pColor2Var = pGlowColour->FindVar("$color2", nullptr))
		{
			pColor2Var->SetVecValue(1.f, 1.f, 1.f);
		}

		StencilStateDisable.SetStencilState(pContext);
		pContext->PopRenderTargetAndViewport();
		pContext->PushRenderTargetAndViewport();
		pContext->Viewport(0, 0, g_ScreenSize.w, g_ScreenSize.h);
		pContext->SetRenderTarget(pRenderBuffer2);
		pContext->DrawScreenSpaceRectangle(pBlurX, 0, 0, g_ScreenSize.w, g_ScreenSize.h, 0.0f, 0.0f, g_ScreenSize.w - 1, g_ScreenSize.h - 1, g_ScreenSize.w, g_ScreenSize.h);
		pContext->SetRenderTarget(pRenderBuffer1);													 										 
		pContext->DrawScreenSpaceRectangle(pBlurY, 0, 0, g_ScreenSize.w, g_ScreenSize.h, 0.0f, 0.0f, g_ScreenSize.w - 1, g_ScreenSize.h - 1, g_ScreenSize.w, g_ScreenSize.h);
		pContext->PopRenderTargetAndViewport();

		ShaderStencilState_t StencilState = {};
		StencilState.m_bEnable = true;
		StencilState.m_nWriteMask = 0x0;
		StencilState.m_nTestMask = 0xFF;
		StencilState.m_nReferenceValue = 0;
		StencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
		StencilState.m_PassOp = STENCILOPERATION_KEEP;
		StencilState.m_FailOp = STENCILOPERATION_KEEP;
		StencilState.m_ZFailOp = STENCILOPERATION_KEEP;
		StencilState.SetStencilState(pContext);

		pContext->DrawScreenSpaceRectangle(pHaloAddToScreen, 0, 0, g_ScreenSize.w, g_ScreenSize.h, 0.0f, 0.0f, g_ScreenSize.w - 1, g_ScreenSize.h - 1, g_ScreenSize.w, g_ScreenSize.h);

		StencilStateDisable.SetStencilState(pContext);

		I::ModelRender->ForcedMaterialOverride(nullptr);
		I::RenderView->SetColorModulation(1.0f, 0.f, 1.f);
		I::RenderView->SetBlend(flOriginalBlend);
	}
	else
	{
		Hook.Original<FN>()(ecx, edx, pViewSetup, bDrawViewmodels);
	}
	g_GlobalInfo.m_bGlowingViewmodel = false;
}