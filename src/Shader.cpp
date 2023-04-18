#include "Shader.h"

#include <filesystem>

Shader::Shader(VkDevice device, std::string fileName, std::string shaderFolder) : device(device) {
	const std::string current_path = std::filesystem::current_path().generic_string();
	fileLocation = current_path + std::string("/") + shaderFolder + fileName;

	if (!std::filesystem::exists(fileLocation)) {
		throw std::runtime_error("File " + fileLocation + " not found!");
	}

	// Infer type from filename
	{
		// Get file extension
		const auto dotIndex = fileLocation.find_last_of('.');
		const auto extension = fileLocation.substr(dotIndex + 1);

		if (extension == "comp")
			type = COMPUTE_SHADER;
		else if (extension == "vert")
			type = VERTEX_SHADER;
		else if (extension == "frag")
			type = FRAGMENT_SHADER;
		else if (extension == "spv")
			type = SPIR_V_BINARY;
	}

	if (type != SPIR_V_BINARY) {
		compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		compileOptions.SetSourceLanguage(shaderc_source_language_glsl);
		compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::vector<std::pair<std::string, std::string>> defs;
		defs.push_back(std::make_pair("__VK_GLSL__", "1"));

		for (auto& defPair : defs) // Add given definitions to compiler
			compileOptions.AddMacroDefinition(defPair.first, defPair.second);
	}

	reload();
}

Shader::~Shader() {
	cleanup();
}

void Shader::reload() {
	// Remove old shader
	cleanup();

	if (type == NONE)
		throw std::runtime_error("Shader has NONE-Type!");

	// Get new shader
	std::vector<char> shaderBinary = readBinaryFile(fileLocation);
	if (type == SPIR_V_BINARY) { // No need to compile
		shaderBinary = readBinaryFile(fileLocation);
	}
	else { // compile GLSL shader
		const std::string sourceString = readTextFile(fileLocation); // Get source of shader

		// preprocess shader
		const shaderc::PreprocessedSourceCompilationResult preprocess = 
			shaderCompiler.PreprocessGlsl(sourceString, (shaderc_shader_kind) type, fileLocation.c_str(), compileOptions);
		if (preprocess.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			throw std::runtime_error("Shader preprocessing failed: " + preprocess.GetErrorMessage());
		}
		const std::string postpre(preprocess.cbegin(), preprocess.cend());

		// compile shader
		const shaderc::SpvCompilationResult binary =
			shaderCompiler.CompileGlslToSpv(postpre, (shaderc_shader_kind)type, fileLocation.c_str(), compileOptions);
		if (binary.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			throw std::runtime_error("Shader compilation failed: " + binary.GetErrorMessage());
		}

		shaderBinary = std::vector<char>(binary.cbegin(), binary.cend());
	}

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.flags = VkShaderModuleCreateFlags();
	createInfo.codeSize = shaderBinary.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBinary.data());

	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Shader Module!");
	}
}