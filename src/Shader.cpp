#include "Shader.h"

#include <iostream>

Shader::Shader(VkDevice device, std::string fileName, std::string shaderFolder) : device(device) {
	const std::string current_path = std::filesystem::current_path().generic_string();
	fileLocation = current_path + std::string("/") + shaderFolder + fileName;

	if (!std::filesystem::exists(fileLocation)) {
		throw std::runtime_error("File " + fileLocation + " not found!");
	}

	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	// Infer type from filename
	{
		// Get file extension
		const auto dotIndex = fileLocation.find_last_of('.');
		const auto extension = fileLocation.substr(dotIndex + 1);

		if (extension == "comp") {
			type = COMPUTE_SHADER;
		}
		else if (extension == "vert") {
			type = VERTEX_SHADER;
			shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		}
		else if (extension == "frag") {
			type = FRAGMENT_SHADER;
			shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else if (extension == "spv") {
			type = SPIR_V_BINARY;
		}
	}

	if (type != SPIR_V_BINARY) {
		// TODO: Add Importer for GLSL Import files
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
	std::cout << "Destroying Shader!" << std::endl;
	cleanup();
}

void Shader::reload() {
	if (type == NONE)
		throw std::runtime_error("Shader has NONE-Type!");

	// check for updated file
	auto last_write = std::filesystem::last_write_time(fileLocation);
	if (last_write == last_updated)
		return; // no update required

	// update last update timestamp
	last_updated = last_write;

	// Get new shader before the old one is removed in case of an error
	std::vector<uint32_t> shaderBinary;
	if (type == SPIR_V_BINARY) { // No need to compile
		std::vector<char> binary = readBinaryFile(fileLocation);
		shaderBinary = std::vector<uint32_t>(reinterpret_cast<uint32_t>(binary.data()), binary.size());
	}
	else { // compile GLSL shader
		const std::string sourceString = readTextFile(fileLocation); // Get source of shader

		// preprocess shader
		const shaderc::PreprocessedSourceCompilationResult preprocess = 
			shaderCompiler.PreprocessGlsl(sourceString, (shaderc_shader_kind) type, fileLocation.c_str(), compileOptions);
		if (preprocess.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << preprocess.GetErrorMessage() << std::endl;
			return; // recompilation failed
		}
		const std::string postpre(preprocess.cbegin(), preprocess.cend());

		// compile shader
		const shaderc::SpvCompilationResult binary =
			shaderCompiler.CompileGlslToSpv(postpre, (shaderc_shader_kind)type, fileLocation.c_str(), compileOptions);
		if (binary.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << binary.GetErrorMessage() << std::endl;
			return; // recompilation failed
		}

		shaderBinary = std::vector<uint32_t>(binary.cbegin(), binary.end());
	}

	// Remove old shader
	cleanup();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.flags = VkShaderModuleCreateFlags();
	createInfo.codeSize = shaderBinary.size() * sizeof(uint32_t);
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBinary.data());

	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Shader Module!");
	}

	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName = "main";

	std::cout << "Successfully loaded Shader: " << fileLocation << std::endl;
}