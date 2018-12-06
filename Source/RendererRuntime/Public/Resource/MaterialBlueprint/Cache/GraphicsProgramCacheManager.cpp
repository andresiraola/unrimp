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


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsProgramCacheManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsProgramCache.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResource.h"
#include "RendererRuntime/Public/Resource/ShaderBlueprint/Cache/ShaderCache.h"
#include "RendererRuntime/Public/Resource/ShaderBlueprint/ShaderBlueprintResourceManager.h"
#include "RendererRuntime/Public/Resource/VertexAttributes/VertexAttributesResourceManager.h"
#include "RendererRuntime/Public/Resource/VertexAttributes/VertexAttributesResource.h"
#include "RendererRuntime/Public/Core/Math/Math.h"
#include "RendererRuntime/Public/IRendererRuntime.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	GraphicsProgramCacheId GraphicsProgramCacheManager::generateGraphicsProgramCacheId(const GraphicsPipelineStateSignature& graphicsPipelineStateSignature)
	{
		GraphicsProgramCacheId graphicsProgramCacheId = Math::FNV1a_INITIAL_HASH_32;
		for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++i)
		{
			const ShaderCombinationId shaderCombinationId = graphicsPipelineStateSignature.getShaderCombinationId(static_cast<GraphicsShaderType>(i));
			if (isValid(shaderCombinationId))
			{
				graphicsProgramCacheId = Math::calculateFNV1a32(reinterpret_cast<const uint8_t*>(&shaderCombinationId), sizeof(ShaderCombinationId), graphicsProgramCacheId);
			}
		}

		// Done
		return graphicsProgramCacheId;
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	GraphicsProgramCache* GraphicsProgramCacheManager::getGraphicsProgramCacheByGraphicsPipelineStateSignature(const GraphicsPipelineStateSignature& graphicsPipelineStateSignature)
	{
		// Does the graphics program cache already exist?
		GraphicsProgramCache* graphicsProgramCache = nullptr;
		const GraphicsProgramCacheId graphicsProgramCacheId = generateGraphicsProgramCacheId(graphicsPipelineStateSignature);
		std::unique_lock<std::mutex> mutexLock(mMutex);
		GraphicsProgramCacheById::const_iterator iterator = mGraphicsProgramCacheById.find(graphicsProgramCacheId);
		if (iterator != mGraphicsProgramCacheById.cend())
		{
			graphicsProgramCache = iterator->second;
		}
		else
		{
			// Create the renderer program: Decide which shader language should be used (for example "GLSL" or "HLSL")
			const MaterialBlueprintResource& materialBlueprintResource = mGraphicsPipelineStateCacheManager.getMaterialBlueprintResource();
			const Renderer::IRootSignaturePtr rootSignaturePtr = materialBlueprintResource.getRootSignaturePtr();
			Renderer::IShaderLanguagePtr shaderLanguage(rootSignaturePtr->getRenderer().getShaderLanguage());
			if (nullptr != shaderLanguage)
			{
				const IRendererRuntime& rendererRuntime = materialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime();

				// Create the shaders
				ShaderCacheManager& shaderCacheManager = rendererRuntime.getShaderBlueprintResourceManager().getShaderCacheManager();
				Renderer::IShader* shaders[NUMBER_OF_GRAPHICS_SHADER_TYPES] = {};
				for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++i)
				{
					ShaderCache* shaderCache = shaderCacheManager.getGraphicsShaderCache(graphicsPipelineStateSignature, materialBlueprintResource, *shaderLanguage, static_cast<GraphicsShaderType>(i));
					if (nullptr != shaderCache)
					{
						shaders[i] = shaderCache->getShaderPtr();
					}
					else
					{
						// No error, just means there's no shader cache because e.g. there's no shader of the requested type
					}
				}

				// Create the graphics program
				Renderer::IGraphicsProgram* graphicsProgram = shaderLanguage->createGraphicsProgram(*rootSignaturePtr,
					rendererRuntime.getVertexAttributesResourceManager().getById(materialBlueprintResource.getVertexAttributesResourceId()).getVertexAttributes(),
					static_cast<Renderer::IVertexShader*>(shaders[static_cast<int>(GraphicsShaderType::Vertex)]),
					static_cast<Renderer::ITessellationControlShader*>(shaders[static_cast<int>(GraphicsShaderType::TessellationControl)]),
					static_cast<Renderer::ITessellationEvaluationShader*>(shaders[static_cast<int>(GraphicsShaderType::TessellationEvaluation)]),
					static_cast<Renderer::IGeometryShader*>(shaders[static_cast<int>(GraphicsShaderType::Geometry)]),
					static_cast<Renderer::IFragmentShader*>(shaders[static_cast<int>(GraphicsShaderType::Fragment)]));
				RENDERER_SET_RESOURCE_DEBUG_NAME(graphicsProgram, "Graphics program cache manager")

				// Create the new graphics program cache instance
				if (nullptr != graphicsProgram)
				{
					graphicsProgramCache = new GraphicsProgramCache(graphicsProgramCacheId, *graphicsProgram);
					mGraphicsProgramCacheById.emplace(graphicsProgramCacheId, graphicsProgramCache);
				}
				else
				{
					// TODO(co) Error handling
					RENDERER_ASSERT(materialBlueprintResource.getResourceManager<MaterialBlueprintResourceManager>().getRendererRuntime().getContext(), false, "Invalid graphics program")
				}
			}
		}

		// Done
		return graphicsProgramCache;
	}

	void GraphicsProgramCacheManager::clearCache()
	{
		std::unique_lock<std::mutex> mutexLock(mMutex);
		for (auto& graphicsProgramCacheElement : mGraphicsProgramCacheById)
		{
			delete graphicsProgramCacheElement.second;
		}
		mGraphicsProgramCacheById.clear();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
