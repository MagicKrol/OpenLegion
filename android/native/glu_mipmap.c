#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
/* Texture input is already in the requested format. GL4ES provides mipmaps. */
GLint gluBuild2DMipmaps(GLenum target, GLint internalFormat, GLsizei width,
                        GLsizei height, GLenum format, GLenum type, const void *data) {
    glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
    glGenerateMipmap(target);
    return glGetError();
}
