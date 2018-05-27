/*********************************************************\
 * Copyright (c) 2012-2018 The Unrimp Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\*********************************************************/


// Amalgamated/unity build
#include "RendererRuntime/PrecompiledHeader.h"
#ifndef ANDROID
	// Not Android source codes
	#include "src/Vr/OpenVR/OpenVRRuntimeLinking.cpp"
	#include "src/Vr/OpenVR/VrManagerOpenVR.cpp"
	#include "src/Vr/OpenVR/Loader/OpenVRMeshResourceLoader.cpp"
	#include "src/Vr/OpenVR/Loader/OpenVRTextureResourceLoader.cpp"
#endif
#ifdef WIN32
	#include "src/PrecompiledHeader.cpp"
#endif
#include "src/Context.cpp"
#include "src/Asset/AssetManager.cpp"
#include "src/Asset/AssetPackage.cpp"
#include "src/Asset/Loader/AssetPackageLoader.cpp"
#include "src/Backend/RendererRuntimeImpl.cpp"
#include "src/Core/File/FileSystemHelper.cpp"
#include "src/Core/File/MemoryFile.cpp"
#include "src/Core/Math/EulerAngles.cpp"
#include "src/Core/Math/Frustum.cpp"
#include "src/Core/Math/Math.cpp"
#include "src/Core/Math/Transform.cpp"
#include "src/Core/Platform/PlatformManager.cpp"
#include "src/Core/Renderer/FramebufferManager.cpp"
#include "src/Core/Renderer/FramebufferSignature.cpp"
#include "src/Core/Renderer/RenderPassManager.cpp"
#include "src/Core/Renderer/RenderTargetTextureManager.cpp"
#include "src/Core/Renderer/RenderTargetTextureSignature.cpp"
#include "src/Core/Time/Stopwatch.cpp"
#include "src/Core/Time/TimeManager.cpp"
#ifdef RENDERER_RUNTIME_IMGUI
	#include "src/DebugGui/DebugGuiHelper.cpp"
	#include "src/DebugGui/DebugGuiManager.cpp"
	#ifdef WIN32
		#include "src/DebugGui/Detail/DebugGuiManagerWindows.cpp"
	#elif UNIX
		#include "src/DebugGui/Detail/DebugGuiManagerLinux.cpp"
	#endif
#endif
#include "src/RenderQueue/Renderable.cpp"
#include "src/RenderQueue/RenderableManager.cpp"
#include "src/RenderQueue/RenderQueue.cpp"
#include "src/Resource/IResourceListener.cpp"
#include "src/Resource/CompositorNode/CompositorNodeInstance.cpp"
#include "src/Resource/CompositorNode/CompositorNodeResource.cpp"
#include "src/Resource/CompositorNode/CompositorNodeResourceManager.cpp"
#include "src/Resource/CompositorNode/CompositorTarget.cpp"
#include "src/Resource/CompositorNode/Loader/CompositorNodeResourceLoader.cpp"
#include "src/Resource/CompositorNode/Pass/CompositorPassFactory.cpp"
#include "src/Resource/CompositorNode/Pass/Clear/CompositorInstancePassClear.cpp"
#include "src/Resource/CompositorNode/Pass/Clear/CompositorResourcePassClear.cpp"
#include "src/Resource/CompositorNode/Pass/Copy/CompositorInstancePassCopy.cpp"
#include "src/Resource/CompositorNode/Pass/Copy/CompositorResourcePassCopy.cpp"
#include "src/Resource/CompositorNode/Pass/DebugGui/CompositorInstancePassDebugGui.cpp"
#include "src/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorInstancePassGenerateMipmaps.cpp"
#include "src/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorResourcePassGenerateMipmaps.cpp"
#include "src/Resource/CompositorNode/Pass/Quad/CompositorInstancePassQuad.cpp"
#include "src/Resource/CompositorNode/Pass/Quad/CompositorResourcePassQuad.cpp"
#include "src/Resource/CompositorNode/Pass/ResolveMultisample/CompositorInstancePassResolveMultisample.cpp"
#include "src/Resource/CompositorNode/Pass/ResolveMultisample/CompositorResourcePassResolveMultisample.cpp"
#include "src/Resource/CompositorNode/Pass/Scene/CompositorInstancePassScene.cpp"
#include "src/Resource/CompositorNode/Pass/Scene/CompositorResourcePassScene.cpp"
#include "src/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.cpp"
#include "src/Resource/CompositorNode/Pass/ShadowMap/CompositorResourcePassShadowMap.cpp"
#include "src/Resource/CompositorNode/Pass/VrHiddenAreaMesh/CompositorInstancePassVrHiddenAreaMesh.cpp"
#include "src/Resource/CompositorNode/Pass/VrHiddenAreaMesh/CompositorResourcePassVrHiddenAreaMesh.cpp"
#include "src/Resource/CompositorWorkspace/CompositorWorkspaceInstance.cpp"
#include "src/Resource/CompositorWorkspace/CompositorWorkspaceResourceManager.cpp"
#include "src/Resource/CompositorWorkspace/Loader/CompositorWorkspaceResourceLoader.cpp"
#include "src/Resource/Detail/IResource.cpp"
#include "src/Resource/Detail/RendererResourceManager.cpp"
#include "src/Resource/Detail/ResourceStreamer.cpp"
#include "src/Resource/Material/MaterialProperties.cpp"
#include "src/Resource/Material/MaterialPropertyValue.cpp"
#include "src/Resource/Material/MaterialResource.cpp"
#include "src/Resource/Material/MaterialResourceManager.cpp"
#include "src/Resource/Material/MaterialTechnique.cpp"
#include "src/Resource/Material/Loader/MaterialResourceLoader.cpp"
#include "src/Resource/MaterialBlueprint/MaterialBlueprintResource.cpp"
#include "src/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.cpp"
#include "src/Resource/MaterialBlueprint/BufferManager/IndirectBufferManager.cpp"
#include "src/Resource/MaterialBlueprint/BufferManager/InstanceBufferManager.cpp"
#include "src/Resource/MaterialBlueprint/BufferManager/LightBufferManager.cpp"
#include "src/Resource/MaterialBlueprint/BufferManager/MaterialBufferManager.cpp"
#include "src/Resource/MaterialBlueprint/BufferManager/MaterialBufferSlot.cpp"
#include "src/Resource/MaterialBlueprint/BufferManager/PassBufferManager.cpp"
#include "src/Resource/MaterialBlueprint/Cache/PipelineStateCacheManager.cpp"
#include "src/Resource/MaterialBlueprint/Cache/PipelineStateCompiler.cpp"
#include "src/Resource/MaterialBlueprint/Cache/PipelineStateSignature.cpp"
#include "src/Resource/MaterialBlueprint/Cache/ProgramCacheManager.cpp"
#include "src/Resource/MaterialBlueprint/Listener/MaterialBlueprintResourceListener.cpp"
#include "src/Resource/MaterialBlueprint/Loader/MaterialBlueprintResourceLoader.cpp"
#include "src/Resource/Mesh/MeshResource.cpp"
#include "src/Resource/Mesh/MeshResourceManager.cpp"
#include "src/Resource/Mesh/Loader/IMeshResourceLoader.cpp"
#include "src/Resource/Mesh/Loader/MeshResourceLoader.cpp"
#include "src/Resource/Scene/SceneNode.cpp"
#include "src/Resource/Scene/SceneResource.cpp"
#include "src/Resource/Scene/SceneResourceManager.cpp"
#include "src/Resource/Scene/Factory/SceneFactory.cpp"
#include "src/Resource/Scene/Item/ISceneItem.cpp"
#include "src/Resource/Scene/Item/MaterialSceneItem.cpp"
#include "src/Resource/Scene/Item/Camera/CameraSceneItem.cpp"
#include "src/Resource/Scene/Item/Light/LightSceneItem.cpp"
#include "src/Resource/Scene/Item/Light/SunlightSceneItem.cpp"
#include "src/Resource/Scene/Item/Mesh/MeshSceneItem.cpp"
#include "src/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.cpp"
#include "src/Resource/Scene/Item/Particles/ParticlesSceneItem.cpp"
#include "src/Resource/Scene/Item/Sky/HosekWilkieSky.cpp"
#include "src/Resource/Scene/Item/Sky/SkySceneItem.cpp"
#include "src/Resource/Scene/Item/Terrain/TerrainSceneItem.cpp"
#include "src/Resource/Scene/Loader/SceneResourceLoader.cpp"
#include "src/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.cpp"
#include "src/Resource/ShaderBlueprint/Cache/Preprocessor/mojoshader_common.cpp"
#include "src/Resource/ShaderBlueprint/Cache/Preprocessor/mojoshader_lexer.cpp"
#include "src/Resource/ShaderBlueprint/Cache/Preprocessor/mojoshader_preprocessor.cpp"
#include "src/Resource/ShaderBlueprint/Cache/Preprocessor/Preprocessor.cpp"
#include "src/Resource/ShaderBlueprint/Cache/ShaderBuilder.cpp"
#include "src/Resource/ShaderBlueprint/Cache/ShaderCacheManager.cpp"
#include "src/Resource/ShaderBlueprint/Cache/ShaderProperties.cpp"
#include "src/Resource/ShaderBlueprint/Loader/ShaderBlueprintResourceLoader.cpp"
#include "src/Resource/ShaderPiece/Loader/ShaderPieceResourceLoader.cpp"
#include "src/Resource/ShaderPiece/ShaderPieceResourceManager.cpp"
#include "src/Resource/Skeleton/SkeletonResource.cpp"
#include "src/Resource/Skeleton/SkeletonResourceManager.cpp"
#include "src/Resource/Skeleton/Loader/SkeletonResourceLoader.cpp"
#include "src/Resource/SkeletonAnimation/SkeletonAnimationController.cpp"
#include "src/Resource/SkeletonAnimation/SkeletonAnimationEvaluator.cpp"
#include "src/Resource/SkeletonAnimation/SkeletonAnimationResourceManager.cpp"
#include "src/Resource/SkeletonAnimation/Loader/SkeletonAnimationResourceLoader.cpp"
#include "src/Resource/Texture/TextureResourceManager.cpp"
#include "src/Resource/Texture/Loader/CrnTextureResourceLoader.cpp"
#include "src/Resource/Texture/Loader/DdsTextureResourceLoader.cpp"
#include "src/Resource/Texture/Loader/ITextureResourceLoader.cpp"
#include "src/Resource/Texture/Loader/KtxTextureResourceLoader.cpp"
#include "src/Resource/VertexAttributes/VertexAttributesResourceManager.cpp"
#include "src/Resource/VertexAttributes/Loader/VertexAttributesResourceLoader.cpp"
