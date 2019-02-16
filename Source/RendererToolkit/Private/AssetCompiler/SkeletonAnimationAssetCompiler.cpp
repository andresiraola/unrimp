/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
#include "RendererToolkit/Private/AssetCompiler/SkeletonAnimationAssetCompiler.h"
#include "RendererToolkit/Private/Helper/AssimpLogStream.h"
#include "RendererToolkit/Private/Helper/AssimpIOSystem.h"
#include "RendererToolkit/Private/Helper/AssimpHelper.h"
#include "RendererToolkit/Private/Helper/CacheManager.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/Asset/AssetPackage.h>
#include <RendererRuntime/Public/Core/File/IFile.h>
#include <RendererRuntime/Public/Core/File/IFileManager.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Public/Core/GetInvalid.h>
#include <RendererRuntime/Public/Resource/SkeletonAnimation/SkeletonAnimationResource.h>
#include <RendererRuntime/Public/Resource/SkeletonAnimation/Loader/SkeletonAnimationFileFormat.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'acl::RotationFormat8::QuatDropW_48' in switch of enum 'acl::RotationFormat8' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4355)	// warning C4355: 'this': used in base member initializer list
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'initializing': conversion from 'int' to 'uint8_t', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'acl::String': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'acl::String': assignment operator was implicitly defined as deleted
	#include <acl/algorithm/uniformly_sampled/encoder.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'FORCE_32BIT' in switch of enum 'aiMetadataType' is not explicitly handled by a case label
	#include <assimp/scene.h>
	#include <assimp/Importer.hpp>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to 'rapidjson::internal::BigInteger::Type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'rapidjson::GenericMember<Encoding,Allocator>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <rapidjson/document.h>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Macros                                                ]
//[-------------------------------------------------------]
#define CENTIMETER_TO_METER(centimeter) (centimeter / 100.0f)


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		class AclAllocator final : public acl::IAllocator
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline explicit AclAllocator(Renderer::IAllocator& allocator) :
				mAllocator(allocator)
			{
				// Nothing here
			}

			explicit AclAllocator(const AclAllocator&) = delete;

			virtual ~AclAllocator() override
			{
				// Nothing here
			}

			AclAllocator& operator=(const AclAllocator&) = delete;


		//[-------------------------------------------------------]
		//[ Public virtual acl::IAllocator methods                ]
		//[-------------------------------------------------------]
		public:
			virtual void* allocate(size_t size, size_t alignment = k_default_alignment) override
			{
				return mAllocator.reallocate(nullptr, 0, size, alignment);
			}

			virtual void deallocate(void* ptr, size_t size) override
			{
				mAllocator.reallocate(ptr, size, 0, 1);
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Renderer::IAllocator& mAllocator;


	};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	std::string SkeletonAnimationAssetCompiler::getVirtualOutputAssetFilename(const Input& input, const Configuration&) const
	{
		return input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string() + ".skeleton_animation";
	}

	bool SkeletonAnimationAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		return input.cacheManager.checkIfFileIsModified(configuration.rendererTarget, input.virtualAssetFilename, {virtualInputFilename}, getVirtualOutputAssetFilename(input, configuration), RendererRuntime::v1SkeletonAnimation::FORMAT_VERSION);
	}

	void SkeletonAnimationAssetCompiler::compile(const Input& input, const Configuration& configuration) const
	{
		// Get relevant data
		const rapidjson::Value& rapidJsonValueSkeletonAnimationAssetCompiler = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"];
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonValue(rapidJsonValueSkeletonAnimationAssetCompiler);
		const std::string virtualOutputAssetFilename = getVirtualOutputAssetFilename(input, configuration);

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		if (input.cacheManager.needsToBeCompiled(configuration.rendererTarget, input.virtualAssetFilename, virtualInputFilename, virtualOutputAssetFilename, RendererRuntime::v1SkeletonAnimation::FORMAT_VERSION, cacheEntries))
		{
			// Create an instance of the Assimp importer class
			AssimpLogStream assimpLogStream;
			Assimp::Importer assimpImporter;
			const RendererToolkit::Context& context = input.context;
			assimpImporter.SetIOHandler(new AssimpIOSystem(context.getFileManager()));

			// Load the given mesh
			const aiScene* assimpScene = assimpImporter.ReadFile(virtualInputFilename.c_str(), AssimpHelper::getAssimpFlagsByRapidJsonValue(rapidJsonValueSkeletonAnimationAssetCompiler, "ImportFlags"));
			if (nullptr != assimpScene && nullptr != assimpScene->mRootNode)
			{
				// Read skeleton animation asset compiler configuration
				uint32_t animationIndex = RendererRuntime::getInvalid<uint32_t>();
				JsonHelper::optionalIntegerProperty(rapidJsonValueSkeletonAnimationAssetCompiler, "AnimationIndex", animationIndex);
				bool ignoreBoneScale = false;
				JsonHelper::optionalBooleanProperty(rapidJsonValueSkeletonAnimationAssetCompiler, "IgnoreBoneScale", ignoreBoneScale);

				// Get the Assimp animation instance to import
				// -> In case there are multiple animations stored inside the imported skeleton animation we must
				//    insist that the skeleton animation compiler gets supplied with the animation index to use
				// -> One skeleton animation assets contains one skeleton animation, everything else would make things more complicated in high-level animation systems
				if (!assimpScene->HasAnimations())
				{
					throw std::runtime_error("The input file \"" + virtualInputFilename + "\" contains no animations");
				}
				if (assimpScene->mNumAnimations > 1)
				{
					if (RendererRuntime::isInvalid(animationIndex))
					{
						throw std::runtime_error("The input file \"" + virtualInputFilename + "\" contains multiple animations, but the skeleton animation compiler wasn't provided with an animation index");
					}
				}
				else
				{
					// "When there's only one candidate, there's only one choice" (Monkey Island 1 quote)
					animationIndex = 0;
				}
				const aiAnimation* assimpAnimation = assimpScene->mAnimations[animationIndex];
				if (0 == assimpAnimation->mNumChannels)
				{
					throw std::runtime_error("The animation at index " + std::to_string(animationIndex) + " of input file \"" + virtualInputFilename + "\" has no channels");
				}

				// Determine whether or not bone scale is used, in case it's not ignored in general to start with
				if (!ignoreBoneScale)
				{
					// Let's be ignorant until someone proofs us wrong
					ignoreBoneScale = true;

					// Try to proof that the guy above is wrong and bone scale is used
					static const aiVector3D ONE_VECTOR(1.0f, 1.0f, 1.0f);
					for (unsigned int channel = 0; channel < assimpAnimation->mNumChannels && ignoreBoneScale; ++channel)
					{
						const aiNodeAnim* assimpNodeAnim = assimpAnimation->mChannels[channel];
						for (unsigned int i = 0; i < assimpNodeAnim->mNumScalingKeys; ++i)
						{
							if (!assimpNodeAnim->mScalingKeys[i].mValue.Equal(ONE_VECTOR, 1e-5f))
							{
								ignoreBoneScale = false;
								break;
							}
						}
					}
				}

				{ // Use ACL ( https://github.com/nfrechette/acl ) to compress the skeleton animation clip
					::detail::AclAllocator aclAllocator(context.getAllocator());
					const uint32_t numberOfSamples = static_cast<uint32_t>(assimpAnimation->mDuration) + 1;

					// Create ACL rigid skeleton
					// -> See ACL documentation https://github.com/nfrechette/acl/blob/develop/docs/creating_a_skeleton.md
					// TODO(co) Fill bone hierarchy
					const uint16_t numberOfBones = static_cast<uint16_t>(assimpAnimation->mNumChannels);
					std::vector<acl::RigidBone> aclRigidBones(numberOfBones);
					for (uint16_t boneIndex = 0; boneIndex < numberOfBones; ++boneIndex)
					{
						acl::RigidBone& aclRigidBone = aclRigidBones[boneIndex];
						#ifdef _DEBUG
						{
							const aiNodeAnim* assimpNodeAnim = assimpAnimation->mChannels[boneIndex];
							aclRigidBone.name = acl::String(aclAllocator, assimpNodeAnim->mNodeName.C_Str());
							RENDERER_ASSERT(context, 1 == assimpNodeAnim->mNumRotationKeys || numberOfSamples == assimpNodeAnim->mNumRotationKeys, "Number of animation rotation keys mismatch")
							RENDERER_ASSERT(context, 1 == assimpNodeAnim->mNumPositionKeys || numberOfSamples == assimpNodeAnim->mNumPositionKeys, "Number of animation position keys mismatch")
							RENDERER_ASSERT(context, ignoreBoneScale || 1 == assimpNodeAnim->mNumScalingKeys || numberOfSamples == assimpNodeAnim->mNumScalingKeys, "Number of animation scaling keys mismatch")
						}
						#endif
						aclRigidBone.vertex_distance = CENTIMETER_TO_METER(3.0f);	// "A value of 3cm is good enough for cinematographic quality for most characters" - https://github.com/nfrechette/acl/blob/develop/docs/creating_a_skeleton.md
					}
					acl::RigidSkeleton aclRigidSkeleton(aclAllocator, aclRigidBones.data(), static_cast<uint16_t>(numberOfBones));

					// Create ACL raw animation clip
					// -> See ACL documentation https://github.com/nfrechette/acl/blob/develop/docs/creating_a_raw_clip.md
					#ifdef _DEBUG
						acl::String name(aclAllocator, assimpAnimation->mName.C_Str());
					#else
						acl::String name;
					#endif
					std::vector<uint32_t> boneIds(numberOfBones);
					acl::AnimationClip aclAnimationClip(aclAllocator, aclRigidSkeleton, numberOfSamples, static_cast<uint32_t>(assimpAnimation->mTicksPerSecond), name);
					{
						// Some Assimp importers like the MD5 one compensate coordinate system differences by setting a root node transform, so we need to take this into account
						const aiQuaternion assimpQuaternionOffset(aiMatrix3x3(assimpScene->mRootNode->mTransformation));
						const bool isMd5 = (aiString("<MD5_Hierarchy>") == assimpScene->mRootNode->mName);

						// Fill ACL raw animation clip
						for (uint16_t boneIndex = 0; boneIndex < numberOfBones; ++boneIndex)
						{
							const aiNodeAnim* assimpNodeAnim = assimpAnimation->mChannels[boneIndex];
							boneIds[boneIndex] = RendererRuntime::StringId::calculateFNV(assimpNodeAnim->mNodeName.C_Str());
							acl::AnimatedBone& aclAnimatedBone = aclAnimationClip.get_animated_bone(boneIndex);

							// Rotation
							// -> Some Assimp importers like the MD5 one compensate coordinate system differences by setting a root node transform, so we need to take this into account
							if (1 == assimpNodeAnim->mNumRotationKeys)
							{
								const aiQuatKey& assimpQuatKey = assimpNodeAnim->mRotationKeys[0];
								aiQuaternion assimpQuaternion = (0 == boneIndex) ? (assimpQuaternionOffset * assimpQuatKey.mValue) : assimpQuatKey.mValue;
								if (!isMd5)
								{
									// TODO(co) Somehow there's a flip when loading OGRE/MD5 skeleton animations. Haven't tried other formats, yet.
									assimpQuaternion.Conjugate();
								}
								const acl::Quat_64 aclQuat_64(acl::quat_set(static_cast<double>(assimpQuaternion.x), static_cast<double>(assimpQuaternion.y), static_cast<double>(assimpQuaternion.z), static_cast<double>(assimpQuaternion.w)));
								acl::AnimationRotationTrack& aclAnimationRotationTrack = aclAnimatedBone.rotation_track;
								for (uint32_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
								{
									aclAnimationRotationTrack.set_sample(sampleIndex, aclQuat_64);
								}
							}
							else if (assimpNodeAnim->mNumRotationKeys > 0)
							{
								acl::AnimationRotationTrack& aclAnimationRotationTrack = aclAnimatedBone.rotation_track;
								for (uint32_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
								{
									const aiQuatKey& assimpQuatKey = assimpNodeAnim->mRotationKeys[sampleIndex];
									aiQuaternion assimpQuaternion = (0 == boneIndex) ? (assimpQuaternionOffset * assimpQuatKey.mValue) : assimpQuatKey.mValue;
									if (!isMd5)
									{
										// TODO(co) Somehow there's a flip when loading OGRE/MD5 skeleton animations. Haven't tried other formats, yet.
										assimpQuaternion.Conjugate();
									}
									aclAnimationRotationTrack.set_sample(sampleIndex, acl::quat_set(static_cast<double>(assimpQuaternion.x), static_cast<double>(assimpQuaternion.y), static_cast<double>(assimpQuaternion.z), static_cast<double>(assimpQuaternion.w)));
								}
							}

							// Translation
							if (1 == assimpNodeAnim->mNumPositionKeys)
							{
								const aiVectorKey& assimpVectorKey = assimpNodeAnim->mPositionKeys[0];
								const acl::Vector4_64 aclVector4_64(acl::vector_set(static_cast<double>(assimpVectorKey.mValue.x), static_cast<double>(assimpVectorKey.mValue.y), static_cast<double>(assimpVectorKey.mValue.z), 0.0));
								acl::AnimationTranslationTrack& aclAnimationTranslationTrack = aclAnimatedBone.translation_track;
								for (uint32_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
								{
									aclAnimationTranslationTrack.set_sample(sampleIndex, aclVector4_64);
								}
							}
							else if (assimpNodeAnim->mNumPositionKeys > 0)
							{
								acl::AnimationTranslationTrack& aclAnimationTranslationTrack = aclAnimatedBone.translation_track;
								for (uint32_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
								{
									const aiVectorKey& assimpVectorKey = assimpNodeAnim->mPositionKeys[sampleIndex];
									aclAnimationTranslationTrack.set_sample(sampleIndex, acl::vector_set(static_cast<double>(assimpVectorKey.mValue.x), static_cast<double>(assimpVectorKey.mValue.y), static_cast<double>(assimpVectorKey.mValue.z), 0.0));
								}
							}

							// Scale
							if (!ignoreBoneScale)
							{
								if (1 == assimpNodeAnim->mNumScalingKeys)
								{
									const aiVectorKey& assimpVectorKey = assimpNodeAnim->mScalingKeys[0];
									const acl::Vector4_64 aclVector4_64(acl::vector_set(static_cast<double>(assimpVectorKey.mValue.x), static_cast<double>(assimpVectorKey.mValue.y), static_cast<double>(assimpVectorKey.mValue.z), 0.0));
									acl::AnimationScaleTrack& aclAnimationScaleTrack = aclAnimatedBone.scale_track;
									for (uint32_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
									{
										aclAnimationScaleTrack.set_sample(sampleIndex, aclVector4_64);
									}
								}
								else if (assimpNodeAnim->mNumScalingKeys > 0)
								{
									acl::AnimationScaleTrack& aclAnimationScaleTrack = aclAnimatedBone.scale_track;
									for (uint32_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
									{
										const aiVectorKey& assimpVectorKey = assimpNodeAnim->mScalingKeys[sampleIndex];
										aclAnimationScaleTrack.set_sample(sampleIndex, acl::vector_set(static_cast<double>(assimpVectorKey.mValue.x), static_cast<double>(assimpVectorKey.mValue.y), static_cast<double>(assimpVectorKey.mValue.z), 0.0));
									}
								}
							}
						}
					}

					// Compress ACL raw animation clip
					// -> See ACL documentation https://github.com/nfrechette/acl/blob/develop/docs/compressing_a_raw_clip.md
					acl::CompressionSettings aclCompressionSettings;
					aclCompressionSettings.rotation_format = acl::RotationFormat8::QuatDropW_Variable;
					aclCompressionSettings.translation_format = acl::VectorFormat8::Vector3_Variable;
					aclCompressionSettings.scale_format = acl::VectorFormat8::Vector3_Variable;
					aclCompressionSettings.range_reduction = acl::RangeReductionFlags8::AllTracks;
					aclCompressionSettings.segmenting.enabled = true;
					aclCompressionSettings.segmenting.range_reduction = acl::RangeReductionFlags8::AllTracks;
					acl::TransformErrorMetric aclErrorMetric;
					aclCompressionSettings.error_metric = &aclErrorMetric;
					aclCompressionSettings.constant_translation_threshold = CENTIMETER_TO_METER(0.001f);
					aclCompressionSettings.error_threshold = CENTIMETER_TO_METER(0.01f);
					acl::OutputStats aclOutputStats;
					acl::CompressedClip* aclCompressedClip = nullptr;
					const acl::ErrorResult aclErrorResult = acl::uniformly_sampled::compress_clip(aclAllocator, aclAnimationClip, aclCompressionSettings, aclCompressedClip, aclOutputStats);
					if (aclErrorResult.any())
					{
						throw std::runtime_error("ACL failed to compress the given skeleton animation clip \"" + virtualInputFilename + "\": " + aclErrorResult.c_str());
					}
					RENDERER_ASSERT(context, nullptr != aclCompressedClip && aclCompressedClip->is_valid(true).empty(), "Compressed ACL clip is invalid");

					// Open file
					// -> There's no need for additional LZ4 compression when using ACL
					RendererRuntime::IFile* file = context.getFileManager().openFile(RendererRuntime::IFileManager::FileMode::WRITE, virtualOutputAssetFilename.c_str());
					if (nullptr == file)
					{
						throw std::runtime_error("Failed to open destination file \"" + std::string(virtualOutputAssetFilename) + '\"');
					}

					{ // Write down the file format header
						struct FileFormatHeader final
						{
							uint32_t formatType;
							uint32_t formatVersion;
						};
						FileFormatHeader fileFormatHeader{RendererRuntime::v1SkeletonAnimation::FORMAT_TYPE, RendererRuntime::v1SkeletonAnimation::FORMAT_VERSION};
						file->write(&fileFormatHeader, sizeof(FileFormatHeader));
					}

					{ // Write down the skeleton animation header
						RendererRuntime::v1SkeletonAnimation::SkeletonAnimationHeader skeletonAnimationHeader;
						skeletonAnimationHeader.numberOfChannels	  = static_cast<uint8_t>(assimpAnimation->mNumChannels);
						skeletonAnimationHeader.durationInTicks		  = static_cast<float>(assimpAnimation->mDuration);
						skeletonAnimationHeader.ticksPerSecond		  = static_cast<float>(assimpAnimation->mTicksPerSecond);
						skeletonAnimationHeader.aclCompressedClipSize = aclCompressedClip->get_size();
						file->write(&skeletonAnimationHeader, sizeof(RendererRuntime::v1SkeletonAnimation::SkeletonAnimationHeader));
					}

					// Write down bone IDs
					file->write(boneIds.data(), sizeof(uint32_t) * numberOfBones);

					// Write down the ACL compressed animation clip
					file->write(aclCompressedClip, aclCompressedClip->get_size());
					aclAllocator.deallocate(aclCompressedClip, aclCompressedClip->get_size());

					// Close file
					context.getFileManager().closeFile(*file);
				}
			}
			else
			{
				throw std::runtime_error("Assimp failed to load in the given skeleton \"" + virtualInputFilename + "\": " + assimpLogStream.getLastErrorMessage());
			}

			// Store new cache entries or update existing ones
			input.cacheManager.storeOrUpdateCacheEntries(cacheEntries);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
