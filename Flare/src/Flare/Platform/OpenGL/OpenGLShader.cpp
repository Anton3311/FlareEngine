#include "OpenGLShader.h"

#include <iostream>
#include <string>
#include <string_view>
#include <fstream>

#include <glad/glad.h>

namespace Flare
{
    OpenGLShader::OpenGLShader(const std::filesystem::path& path)
    {
        std::string source;
        std::ifstream file(path);
        if (file.is_open())
        {
            file.seekg(0, std::ios::end);
            source.resize(file.tellg());
            file.seekg(0, std::ios::beg);
            file.read(&source[0], source.size());
            file.close();

            Compile(source);
        }
        else
            std::cout << "Failed to open shader file";
    }
    
    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_Id);
    }

    void Flare::OpenGLShader::Bind()
    {
        glUseProgram(m_Id);
    }

    std::vector<OpenGLShader::ShaderProgram> OpenGLShader::PreProcess(std::string_view source)
    {
        std::vector<ShaderProgram> programs;

        std::string_view typeToken = "#type";

        size_t position = source.find_first_of(typeToken);
        while (position != std::string_view::npos)
        {
            size_t endOfLine = source.find_first_of("\n", position);
            size_t typeStart = position + typeToken.size() + 1;
            size_t nextLineStart = source.find_first_of("\n", endOfLine);

            std::string_view type = source.substr(typeStart, endOfLine - typeStart);

            GLenum shaderType;
            if (type == "vertex")
                shaderType = GL_VERTEX_SHADER;
            else if (type == "fragment")
                shaderType = GL_FRAGMENT_SHADER;
            else
                std::cout << "Invalid shader type\n";

            position = source.find(typeToken, nextLineStart);

            std::string_view programSource;
            if (position == std::string_view::npos)
                programSource = source.substr(nextLineStart);
            else
                programSource = source.substr(nextLineStart, position - nextLineStart);

            programs.emplace_back(programSource, shaderType);
        }

        return programs;
    }

    void OpenGLShader::Compile(std::string_view source)
    {
        m_Id = glCreateProgram();
        auto programs = PreProcess(source);

        std::vector<uint32_t> shaderIds(programs.size());

        for (auto& program : programs)
        {
            const char* source = program.Source.c_str();

            GLuint shader = glCreateShader(program.Type);
            glShaderSource(shader, 1, &source, 0);
            glCompileShader(shader);

            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE)
            {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
                glDeleteShader(shader);

                std::cout << infoLog.data() << '\n';
                break;
            }

            glAttachShader(m_Id, shader);
            shaderIds.push_back(shader);
        }

        glLinkProgram(m_Id);
        GLint isLinked = 0;
        glGetProgramiv(m_Id, GL_LINK_STATUS, (int*)&isLinked);

        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength + 1);
            glGetProgramInfoLog(m_Id, maxLength, &maxLength, &infoLog[0]);
            glDeleteProgram(m_Id);

            std::cout << infoLog.data() << '\n';

            for (auto id : shaderIds)
                glDeleteShader(id);
            return;
        }

        for (auto id : shaderIds)
            glDetachShader(m_Id, id);
    }
}