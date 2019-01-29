#pragma once

#include <GL/gl3w.h>
#include <lm/logger.h>
#include <lm/math.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// --------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(gl)

static void CheckGLErrors(const char* filename, const int line) {
    if (int err = glGetError(); err != GL_NO_ERROR) {
        std::string errstr;
        switch (err) {
            case GL_INVALID_ENUM:                    { errstr = "GL_INVALID_ENUM"; break; }
            case GL_INVALID_VALUE:                   { errstr = "GL_INVALID_VALUE"; break; }
            case GL_INVALID_OPERATION:               { errstr = "GL_INVALID_OPERATION"; break; }
            case GL_INVALID_FRAMEBUFFER_OPERATION:   { errstr = "GL_INVALID_FRAMEBUFFER_OPERATION"; break; }
            case GL_OUT_OF_MEMORY:                   { errstr = "GL_OUT_OF_MEMORY"; break; }
        }
        LM_ERROR("{} {} {}", errstr, filename, line);
    }
}

static void DebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar* message, const GLvoid* /*userParam*/) {
    std::string sourceString;
    std::string typeString;
    std::string severityString;
    switch (source) {
        case GL_DEBUG_SOURCE_API:                { sourceString = "OpenGL"; break; }
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:      { sourceString = "Windows"; break; }
        case GL_DEBUG_SOURCE_SHADER_COMPILER:    { sourceString = "Shader Compiler"; break; }
        case GL_DEBUG_SOURCE_THIRD_PARTY:        { sourceString = "Third Party"; break; }
        case GL_DEBUG_SOURCE_APPLICATION:        { sourceString = "Application"; break; }
        case GL_DEBUG_SOURCE_OTHER:              { sourceString = "Other"; break; }
    }
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:                { typeString = "Error"; break; }
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:  { typeString = "Deprecated behavior"; break; }
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:   { typeString = "Undefined behavior"; break; }
        case GL_DEBUG_TYPE_PORTABILITY:          { typeString = "Portability"; break; }
        case GL_DEBUG_TYPE_PERFORMANCE:          { typeString = "Performance"; break; }
        case GL_DEBUG_TYPE_OTHER:                { typeString = "Message"; break; }
        case GL_DEBUG_TYPE_MARKER:               { typeString = "Marker"; break; }
        case GL_DEBUG_TYPE_PUSH_GROUP:           { typeString = "Push group"; break; }
        case GL_DEBUG_TYPE_POP_GROUP:            { typeString = "Pop group"; break; }
    }
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:             { severityString = "High"; break; }
        case GL_DEBUG_SEVERITY_MEDIUM:           { severityString = "Medium"; break; }
        case GL_DEBUG_SEVERITY_LOW:              { severityString = "Low"; break; }
        case GL_DEBUG_SEVERITY_NOTIFICATION:     { severityString = "Notification"; break; }
    }
    LM_INFO("{}: {}({}) {}: {}", sourceString, typeString, severityString, id, message);
}

#define LM_GL_CHECK_ERRORS() gl::CheckGLErrors(__FILE__, __LINE__)
#define LM_GL_OFFSET_OF(Type, Element) ((size_t)&(((Type*)0)->Element))

LM_NAMESPACE_END(gl)

// ----------------------------------------------------------------------------

namespace GLResourceType {
    enum {
        None                = 0,
        Pipeline            = 1<<0,
        Program             = 1<<1,
        ArrayBuffer         = 1<<2,
        ElementArrayBuffer  = 1<<3,
        VertexArray         = 1<<4,
        Texture2D           = 1<<5,
        Buffer              = ArrayBuffer | ElementArrayBuffer,
        Texture             = Texture2D,
        Bindable            = Pipeline | Texture
    };
};

class GLResource {
private:
    int type_ = GLResourceType::None;
    GLuint name_;

private:
    struct {
        struct {
            GLbitfield stages;
            std::unordered_map<std::string, GLuint> uniformLocationMap;
        } program;
        struct {
            GLenum target;
        } buffer;
        struct {
            GLenum target;
        } texture;
    } data_;

public:
    #pragma region Create & Destoroy
    void Create(int type) {
        type_ = type;
        if (type_ == GLResourceType::Pipeline) {
            glGenProgramPipelines(1, &name_);
        }
        else if (type_ == GLResourceType::Program) {
            name_ = glCreateProgram();
            data_.program.stages = 0;
        }
        else if ((type_ & GLResourceType::Buffer) > 0) {
            glGenBuffers(1, &name_);
            if (type_ == GLResourceType::ArrayBuffer) {
                data_.buffer.target = GL_ARRAY_BUFFER;
            }
            else if (type_ == GLResourceType::ElementArrayBuffer) {
                data_.buffer.target = GL_ELEMENT_ARRAY_BUFFER;
            }
        }
        else if (type_ == GLResourceType::VertexArray) {
            glGenVertexArrays(1, &name_);
        }
        else if ((type_ & GLResourceType::Texture) > 0) {
            glGenTextures(1, &name_);
            if (type_ == GLResourceType::Texture2D) {
                data_.texture.target = GL_TEXTURE_2D;
            }
        }
        LM_GL_CHECK_ERRORS();
    }

    void Destory()
    {
        if (type_ == GLResourceType::Pipeline)            { glDeleteProgramPipelines(1, &name_); }
        else if (type_ == GLResourceType::Program)        { glDeleteProgram(name_); }
        else if ((type_ & GLResourceType::Buffer) > 0)    { glDeleteBuffers(1, &name_); }
        else if (type_ == GLResourceType::VertexArray)    { glDeleteVertexArrays(1, &name_); }
        else if (type_ == GLResourceType::Texture)        { glDeleteTextures(1, &name_); }
        LM_GL_CHECK_ERRORS();
    }
    #pragma endregion

public:
    #pragma region Bindable
    void Bind() const {
        if ((type_ & GLResourceType::Bindable) == 0) { LM_ERROR("Invalid type"); return; }
        if (type_ == GLResourceType::Pipeline) { glBindProgramPipeline(name_); }
        if ((type_ & GLResourceType::Texture) > 0) { glBindTexture(data_.texture.target, name_); }
        LM_GL_CHECK_ERRORS();
    }

    void Unbind() const {
        if ((type_ & GLResourceType::Bindable) == 0) { LM_ERROR("Invalid type"); return; }
        if (type_ == GLResourceType::Pipeline) { glBindProgramPipeline(0); }
        if ((type_ & GLResourceType::Texture) > 0) { glBindTexture(data_.texture.target, 0); }
        LM_GL_CHECK_ERRORS();
    }
    #pragma endregion

public:
    #pragma region Getter
    int GetType()        { return type_; }
    GLuint GetName()    { return name_; }
    #pragma endregion

public:
    #pragma region Program type specific functions
    bool CompileString(GLenum shaderType, const std::string& content) {
        if (type_ != GLResourceType::Program) {
            LM_ERROR("Invalid type");
            return false;
        }

        // Create & Compile
        GLuint shaderID = glCreateShader(shaderType);
        const char* contentPtr = content.c_str();

        glShaderSource(shaderID, 1, &contentPtr, NULL);
        glCompileShader(shaderID);

        GLint ret;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &ret);
        if (ret == 0) {
            int length;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);

            std::vector<char> v(length);
            glGetShaderInfoLog(shaderID, length, NULL, &v[0]);
            glDeleteShader(shaderID);

            LM_ERROR(std::string(&v[0]));
            return false;
        }

        // Attach to program
        glAttachShader(name_, shaderID);
        glProgramParameteri(name_, GL_PROGRAM_SEPARABLE, GL_TRUE);
        glDeleteShader(shaderID);

        // Add a flag for later use
        switch (shaderType) {
            case GL_VERTEX_SHADER:          { data_.program.stages |= GL_VERTEX_SHADER_BIT; break; }
            case GL_TESS_CONTROL_SHADER:    { data_.program.stages |= GL_TESS_CONTROL_SHADER_BIT; break; }
            case GL_TESS_EVALUATION_SHADER: { data_.program.stages |= GL_TESS_EVALUATION_SHADER_BIT; break; }
            case GL_GEOMETRY_SHADER:        { data_.program.stages |= GL_GEOMETRY_SHADER_BIT; break; }
            case GL_FRAGMENT_SHADER:        { data_.program.stages |= GL_FRAGMENT_SHADER_BIT; break; }
            case GL_COMPUTE_SHADER:         { data_.program.stages |= GL_COMPUTE_SHADER_BIT; break; }
        }

        LM_GL_CHECK_ERRORS();
        return true;
    }

    bool Link() {
        if (type_ != GLResourceType::Program) {
            LM_ERROR("Invalid type");
            return false;
        }

        glLinkProgram(name_);

        GLint ret;
        glGetProgramiv(name_, GL_LINK_STATUS, &ret);

        if (ret == GL_FALSE) {
            GLint length;
            glGetProgramiv(name_, GL_INFO_LOG_LENGTH, &length);

            std::vector<char> v(length);
            glGetProgramInfoLog(name_, length, NULL, &v[0]);

            LM_ERROR(std::string(&v[0]));
            return false;
        }

        LM_GL_CHECK_ERRORS();
        return true;
    }

    void SetUniform(const std::string& name, int v) {
        if (type_ != GLResourceType::Program) { LM_ERROR("Invalid type"); return; }
        glProgramUniform1i(name_, GetOrCreateUniformName(name), v);
        LM_GL_CHECK_ERRORS();
    }

    void SetUniform(const std::string& name, float v) {
        if (type_ != GLResourceType::Program) { LM_ERROR("Invalid type"); return; }
        glProgramUniform1f(name_, GetOrCreateUniformName(name), v);
        LM_GL_CHECK_ERRORS();
    }

    void SetUniform(const std::string& name, const glm::vec3& v) {
        if (type_ != GLResourceType::Program) { LM_ERROR("Invalid type"); return; }
        glProgramUniform3fv(name_, GetOrCreateUniformName(name), 1, &v.x);
        LM_GL_CHECK_ERRORS();
    }

    void SetUniform(const std::string& name, const glm::vec4& v) {
        if (type_ != GLResourceType::Program) { LM_ERROR("Invalid type"); return; }
        glProgramUniform4fv(name_, GetOrCreateUniformName(name), 1, &v.x);
        LM_GL_CHECK_ERRORS();
    }

    void SetUniform(const std::string& name, const glm::mat4& mat) {
        if (type_ != GLResourceType::Program) { LM_ERROR("Invalid type"); return; }
        glProgramUniformMatrix4fv(name_, GetOrCreateUniformName(name), 1, GL_FALSE, glm::value_ptr(mat));
        LM_GL_CHECK_ERRORS();
    }

    void SetUniform(const std::string& name, const float* mat) {
        if (type_ != GLResourceType::Program) { LM_ERROR("Invalid type"); return; }
        glProgramUniformMatrix4fv(name_, GetOrCreateUniformName(name), 1, GL_FALSE, mat);
        LM_GL_CHECK_ERRORS();
    }

    GLuint GetOrCreateUniformName(const std::string& name) {
        if (type_ != GLResourceType::Program) {
            LM_ERROR("Invalid type");
            return 0;
        }
        
        auto& locationMap = data_.program.uniformLocationMap;
        auto it = locationMap.find(name);
        if (it == locationMap.end()) {
            GLuint loc = glGetUniformLocation(name_, name.c_str());
            locationMap[name] = loc;
            return loc;
        }

        return it->second;
    }
    #pragma endregion

public:
    #pragma region Pipeline type specific functions
    void AddProgram(const GLResource& program) {
        if (type_ != GLResourceType::Pipeline || program.type_ != GLResourceType::Program) {
            LM_ERROR("Invalid type");
            return;
        }
        glUseProgramStages(name_, program.data_.program.stages, program.name_);
        LM_GL_CHECK_ERRORS();
    }
    #pragma endregion

public:

    #pragma region Buffer type specific functions
    bool Allocate(GLsizeiptr size, const GLvoid* data, GLenum usage) {
        if ((type_ & GLResourceType::Buffer) == 0) { LM_ERROR("Invalid type"); return false; }
        glBindBuffer(data_.buffer.target, name_);
        glBufferData(data_.buffer.target, size, data, usage);
        glBindBuffer(data_.buffer.target, 0);
        LM_GL_CHECK_ERRORS();
        return true;
    }

    void* MapBuffer(GLenum access) const {
        if ((type_ & GLResourceType::Buffer) == 0) { LM_ERROR("Invalid type"); return nullptr; }
        glBindBuffer(data_.buffer.target, name_);
        auto* p = glMapBuffer(data_.buffer.target, access);
        LM_GL_CHECK_ERRORS();
        return p;
    }

    void UnmapBuffer() const {
        if ((type_ & GLResourceType::Buffer) == 0) { LM_ERROR("Invalid type"); return; }
        glUnmapBuffer(data_.buffer.target);
        glBindBuffer(data_.buffer.target, 0);
        LM_GL_CHECK_ERRORS();
    }

    int BufferSize() const {
        if ((type_ & GLResourceType::Buffer) == 0) { LM_ERROR("Invalid type"); return false; }
        GLint v;
        glBindBuffer(data_.buffer.target, name_);
        glGetBufferParameteriv(data_.buffer.target, GL_BUFFER_SIZE, &v);
        glBindBuffer(data_.buffer.target, 0);
        LM_GL_CHECK_ERRORS();
        return v;
    }
    #pragma endregion

public:

    #pragma region Vertex array type specific functions
    void AddVertexAttribute(const GLResource& v, GLuint index, GLuint componentNum, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* start) const {
        if (type_ != GLResourceType::VertexArray || v.type_ != GLResourceType::ArrayBuffer) {
            LM_ERROR("Invalid type");
            return;
        }
        glBindVertexArray(name_);
        glBindBuffer(GL_ARRAY_BUFFER, v.name_);
        glVertexAttribPointer(index, componentNum, type, normalized, stride, start);
        glEnableVertexAttribArray(index);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        LM_GL_CHECK_ERRORS();
    }

    void Draw(GLenum mode, int offset, int count) const {
        if (type_ != GLResourceType::VertexArray) {
            LM_ERROR("Invalid type");
            return;
        }
        glBindVertexArray(name_);
        glDrawArrays(mode, offset, count);
        glBindVertexArray(0);
        LM_GL_CHECK_ERRORS();
    }

    void Draw(GLenum mode, const GLResource& ibo, int count) const {
        if (type_ != GLResourceType::VertexArray || ibo.type_ != GLResourceType::ElementArrayBuffer) {
            LM_ERROR("Invalid type");
            return;
        }
        glBindVertexArray(name_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo.name_);
        glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        LM_GL_CHECK_ERRORS();
    }

    void Draw(GLenum mode, const GLResource& ibo) const {
        Draw(mode, ibo, ibo.BufferSize() / sizeof(GLuint));
    }
    #pragma endregion
};

LM_NAMESPACE_END(LM_NAMESPACE)
