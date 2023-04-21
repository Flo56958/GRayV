#include <string>
#include <fstream>
#include <Vulkan/Vulkan.hpp>
#include <shaderc/shaderc.hpp>
#include <filesystem>

enum ShaderType {
    NONE = -1,
    SPIR_V_BINARY = -2,
    VERTEX_SHADER = shaderc_shader_kind::shaderc_glsl_default_vertex_shader,
    FRAGMENT_SHADER = shaderc_shader_kind::shaderc_glsl_default_fragment_shader,
    COMPUTE_SHADER = shaderc_shader_kind::shaderc_glsl_default_compute_shader
};

class Shader {
public:
    Shader(VkDevice device, std::string fileName, std::string shaderFolder = "/../shader/");
    ~Shader();

    void reload();

    ShaderType getType() { return type; }
    VkPipelineShaderStageCreateInfo getShaderStageInfo() { return shaderStageInfo; }

private:
    ShaderType type = NONE;
    std::string fileLocation;

    VkDevice device;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    std::filesystem::file_time_type last_updated;

    shaderc::Compiler shaderCompiler;
    shaderc::CompileOptions compileOptions;

    std::vector<char> readBinaryFile(const std::string& fileName) {
        std::ifstream fileStream(fileName, std::ios::binary | std::ios::in | std::ios::ate);
        if (!fileStream.is_open())
            throw std::runtime_error("Could not open file " + fileName + "!");

        const size_t size = fileStream.tellg();
        fileStream.seekg(0, std::ios::beg);
        std::vector<char> data(size);
        fileStream.read(data.data(), size);
        fileStream.close();
        return data;
    }

    std::string readTextFile(const std::string& fileName) {
        std::string buffer;
        std::ifstream fileStream(fileName.data());
        if (!fileStream.is_open())
            throw std::runtime_error("Could not open file " + fileName + "!");

        std::string temp;
        while (getline(fileStream, temp))
            buffer.append(temp), buffer.append("\n");

        fileStream >> buffer;
        fileStream.close();
        return buffer;
    }

    void cleanup() {
        if (shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, shaderModule, nullptr);
        }
    }
};