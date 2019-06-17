#ifndef GLFUNC_H
#define GLFUNC_H

#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

namespace olive {
namespace gl {

/**
 * @brief Draw texture on screen
 *
 * @param pipeline
 *
 * Shader to use for the texture drawing
 *
 * @param flipped
 *
 * Draw the texture vertically flipped (defaults to FALSE)
 *
 * @param matrix
 *
 * Transformation matrix to use when drawing (defaults to no transform)
 */
void Blit(QOpenGLShaderProgram* pipeline, bool flipped = false, QMatrix4x4 matrix = QMatrix4x4());

}
}

#endif // GLFUNC_H
