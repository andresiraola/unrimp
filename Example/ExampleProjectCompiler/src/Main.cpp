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
#include "CommandLineArguments.h"

#include <RendererToolkit/Public/RendererToolkitInstance.h>

#include <RendererRuntime/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Core/File/DefaultFileManager.h>

#include <Renderer/DefaultLog.h>
#include <Renderer/DefaultAssert.h>
#include <Renderer/DefaultAllocator.h>

#include <iostream>


//[-------------------------------------------------------]
//[ Platform independent program entry point              ]
//[-------------------------------------------------------]
int programEntryPoint(const CommandLineArguments& commandLineArguments)
{
	Renderer::DefaultLog defaultLog;
	Renderer::DefaultAssert defaultAssert;
	Renderer::DefaultAllocator defaultAllocator;
	RendererRuntime::DefaultFileManager defaultFileManager(defaultLog, defaultAssert, defaultAllocator, (std_filesystem::canonical(std_filesystem::current_path()) / "..").generic_string());
	RendererToolkit::Context rendererToolkitContext(defaultLog, defaultAssert, defaultAllocator, defaultFileManager);
	RendererToolkit::RendererToolkitInstance rendererToolkitInstance(rendererToolkitContext);
	RendererToolkit::IRendererToolkit* rendererToolkit = rendererToolkitInstance.getRendererToolkit();
	if (nullptr != rendererToolkit)
	{
		RendererToolkit::IProject* project = rendererToolkit->createProject();
		try
		{
			project->load("../DataSource/Example");

			if (commandLineArguments.getArguments().empty())
			{
				//	project->compileAllAssets("Direct3D9_30");
					project->compileAllAssets("Direct3D11_50");
				//	project->compileAllAssets("Direct3D12_50");
				//	project->compileAllAssets("OpenGLES3_300");
				//	project->compileAllAssets("OpenGL_440");
			}
			else
			{
				// For now all given arguments are interpreted as render target
				for (const std::string& renderTarget : commandLineArguments.getArguments())
				{
					RENDERER_LOG(rendererToolkitContext, INFORMATION, "Compiling for target: \"%s\"", renderTarget.c_str())
					project->compileAllAssets(renderTarget.c_str());
					RENDERER_LOG(rendererToolkitContext, INFORMATION, "Compilation done")
				}
			}
		}
		catch (const std::exception& e)
		{
			RENDERER_LOG(rendererToolkitContext, CRITICAL, "Project compilation failed: %s", e.what())
			RENDERER_LOG(rendererToolkitContext, INFORMATION, "Press any key to continue")
			getchar();
		}
		delete project;
	}

	// No error
	return 0;
}


//[-------------------------------------------------------]
//[ Platform dependent program entry point                ]
//[-------------------------------------------------------]
// Windows implementation
#ifdef _WIN32

	// For memory leak detection
	#ifdef _DEBUG
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif

	#ifdef _CONSOLE
		#ifdef UNICODE
			int wmain(int, wchar_t**)
		#else
			int main(int, char**)
		#endif
			{
				// Call the platform independent program entry point
				// -> Uses internally "GetCommandLine()" to fetch the command line arguments
				const int result = programEntryPoint(CommandLineArguments());

				// For memory leak detection
				#ifdef _DEBUG
					_CrtDumpMemoryLeaks();
				#endif

				// Done
				return result;
			}
	#else
		#include <RendererRuntime/Core/Platform/WindowsHeader.h>

		#ifdef UNICODE
			int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
		#else
			int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
		#endif
			{
				// Call the platform independent program entry point
				// -> Uses internally "GetCommandLine()" to fetch the command line arguments
				const int result = programEntryPoint(CommandLineArguments());

				// For memory leak detection
				#ifdef _DEBUG
					_CrtDumpMemoryLeaks();
				#endif

				// Done
				return result;
			}
	#endif

// Linux implementation
#elif LINUX
	int main(int argc, char** argv)
	{
		// Call the platform independent program entry point
		return programEntryPoint(CommandLineArguments(argc, argv));
	}
#endif
