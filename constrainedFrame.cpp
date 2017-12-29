/****************************************************************************

 Copyright (C) 2002-2014 Gilles Debunne. All rights reserved.

 This file is part of the QGLViewer library version 2.7.1.

 http://www.libqglviewer.com - contact@libqglviewer.com

 This file may be used under the terms of the GNU General Public License 
 versions 2.0 or 3.0 as published by the Free Software Foundation and
 appearing in the LICENSE file included in the packaging of this file.
 In addition, as a special exception, Gilles Debunne gives you certain 
 additional rights, described in the file GPL_EXCEPTION in this package.

 libQGLViewer uses dual licensing. Commercial/proprietary software must
 purchase a libQGLViewer Commercial License.

 This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

*****************************************************************************/

#include "constrainedFrame.h"

#include <QGLViewer/manipulatedCameraFrame.h>
#include <QKeyEvent>

using namespace qglviewer;
using namespace std;

AxisPlaneConstraint::Type
Viewer::nextTranslationConstraintType(const AxisPlaneConstraint::Type &type) {
  switch (type) {
  case AxisPlaneConstraint::FREE:
    return AxisPlaneConstraint::PLANE;
    break;
  case AxisPlaneConstraint::PLANE:
    return AxisPlaneConstraint::AXIS;
    break;
  case AxisPlaneConstraint::AXIS:
    return AxisPlaneConstraint::FORBIDDEN;
    break;
  case AxisPlaneConstraint::FORBIDDEN:
    return AxisPlaneConstraint::FREE;
    break;
  default:
    return AxisPlaneConstraint::FREE;
  }
}

AxisPlaneConstraint::Type
Viewer::nextRotationConstraintType(const AxisPlaneConstraint::Type &type) {
  switch (type) {
  case AxisPlaneConstraint::FREE:
    return AxisPlaneConstraint::AXIS;
    break;
  case AxisPlaneConstraint::AXIS:
    return AxisPlaneConstraint::FORBIDDEN;
    break;
  case AxisPlaneConstraint::PLANE:
  case AxisPlaneConstraint::FORBIDDEN:
    return AxisPlaneConstraint::FREE;
    break;
  default:
    return AxisPlaneConstraint::FREE;
  }
}

void Viewer::changeConstraint() {
  unsigned short previous = activeConstraint;
  activeConstraint = (activeConstraint + 1) % 3;

  constraints[activeConstraint]->setTranslationConstraintType(
      constraints[previous]->translationConstraintType());
  constraints[activeConstraint]->setTranslationConstraintDirection(
      constraints[previous]->translationConstraintDirection());
  constraints[activeConstraint]->setRotationConstraintType(
      constraints[previous]->rotationConstraintType());
  constraints[activeConstraint]->setRotationConstraintDirection(
      constraints[previous]->rotationConstraintDirection());

  frame->setConstraint(constraints[activeConstraint]);
}

void Viewer::init() {
  constraints[0] = new LocalConstraint();
  constraints[1] = new WorldConstraint();
  constraints[2] = new CameraConstraint(camera());

  transDir = 0; // X direction
  rotDir = 0;   // X direction
  activeConstraint = 0;

  frame = new ManipulatedFrame();
  setManipulatedFrame(frame);
  frame->setConstraint(constraints[activeConstraint]);

  //AltModifier 调节器(alt + 鼠标事件)
  //绑定鼠标事件 左键控制摄像机旋转  (alt + 鼠标左键):摄像机平移
  setMouseBinding(Qt::AltModifier, Qt::LeftButton, QGLViewer::CAMERA,
                  QGLViewer::ROTATE);
  //绑定鼠标事件 右键控制摄像机平移  (alt + 鼠标右键):出现一个对话框，可选择最大最小等设置
  setMouseBinding(Qt::AltModifier, Qt::RightButton, QGLViewer::CAMERA,
                  QGLViewer::TRANSLATE);
  //绑定鼠标事件 滑轮控制摄像机远近(整个窗口上物体的缩放) (alt + 鼠标滑轮):摄像机缩放
  setMouseBinding(Qt::AltModifier, Qt::MidButton, QGLViewer::CAMERA,
                  QGLViewer::ZOOM);
  //绑定鼠标滑轮事件 滑轮控制摄像机远近(整个窗口上物体的缩放)
  setWheelBinding(Qt::AltModifier, QGLViewer::CAMERA, QGLViewer::ZOOM);


  //只有鼠标事件 设置frame图像的状态
  setMouseBinding(Qt::NoModifier, Qt::LeftButton, QGLViewer::FRAME,
                  QGLViewer::ROTATE);
  setMouseBinding(Qt::NoModifier, Qt::RightButton, QGLViewer::FRAME,
                  QGLViewer::TRANSLATE);
  setMouseBinding(Qt::NoModifier, Qt::MidButton, QGLViewer::FRAME,
                  QGLViewer::ZOOM);
  setWheelBinding(Qt::NoModifier, QGLViewer::FRAME, QGLViewer::ZOOM);

  //shift和只有鼠标事件一样
  setMouseBinding(Qt::ShiftModifier, Qt::LeftButton, QGLViewer::FRAME,
                  QGLViewer::ROTATE, false);
  setMouseBinding(Qt::ShiftModifier, Qt::RightButton, QGLViewer::FRAME,
                  QGLViewer::TRANSLATE, false);
  setMouseBinding(Qt::ShiftModifier, Qt::MidButton, QGLViewer::FRAME,
                  QGLViewer::ZOOM, false);
  setWheelBinding(Qt::ShiftModifier, QGLViewer::FRAME, QGLViewer::ZOOM, false);

  setAxisIsDrawn();//绘制轴

  setKeyDescription(Qt::Key_G, "Change translation constraint direction");
  setKeyDescription(Qt::Key_D, "Change rotation constraint direction");
  setKeyDescription(Qt::Key_Space, "Change constraint reference");
  setKeyDescription(Qt::Key_T, "Change translation constraint type");
  setKeyDescription(Qt::Key_R, "Change rotation constraint type");

  restoreStateFromFile();//刷新viewer界面
  help();
}

void Viewer::draw() {
  glMultMatrixd(frame->matrix());//矩阵的乘法
  drawAxis(0.4f);//绘制轴
  const float scale = 0.3f;
  //物体的每个点的x,y,z坐标与对应的xyz参数相乘
  //参数也可取负数，也可以理解为先关于某轴翻转180°，再缩放
  glScalef(scale, scale, scale);

  const float nbSteps = 200.0;
  glBegin(GL_QUAD_STRIP);//绘制螺旋模型
  for (float i = 0; i < nbSteps; ++i) {
    float ratio = i / nbSteps;
    float angle = 21.0 * ratio;
    float c = cos(angle);
    float s = sin(angle);
    float r1 = 1.0 - 0.8 * ratio;
    float r2 = 0.8 - 0.8 * ratio;
    float alt = ratio - 0.5;
    const float nor = .5;
    const float up = sqrt(1.0 - nor * nor);
    glColor3f(1 - ratio, 0.2f, ratio);
    glNormal3f(nor * c * scale, up * scale, nor * s * scale);
    glVertex3f(r1 * c, alt, r1 * s);
    glVertex3f(r2 * c, alt + 0.05, r2 * s);
  }
  glEnd();

  displayText();//绘制文字
}

void Viewer::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
  case Qt::Key_G://键盘事件'G' 设置平移轴
    transDir = (transDir + 1) % 3;
    break;
  case Qt::Key_D://键盘事件'D' 设置旋转轴
    rotDir = (rotDir + 1) % 3;
    break;
  case Qt::Key_Space://键盘事件'空格' 设置平移/旋转矩阵
    changeConstraint();
    break;
  case Qt::Key_T://键盘事件'T' 设置平移矩阵
    constraints[activeConstraint]->setTranslationConstraintType(
        nextTranslationConstraintType(
            constraints[activeConstraint]->translationConstraintType()));
    break;
  case Qt::Key_R://键盘事件'R' 设置旋转矩阵
    constraints[activeConstraint]->setRotationConstraintType(
        nextRotationConstraintType(
            constraints[activeConstraint]->rotationConstraintType()));
    break;
  default:
    QGLViewer::keyPressEvent(e);
  }

  Vec dir(0.0, 0.0, 0.0);
  dir[transDir] = 1.0;//设置平移轴
  constraints[activeConstraint]->setTranslationConstraintDirection(dir);

  dir = Vec(0.0, 0.0, 0.0);
  dir[rotDir] = 1.0;//设置旋转轴
  constraints[activeConstraint]->setRotationConstraintDirection(dir);

  update();//更新界面
}

void Viewer::displayType(const AxisPlaneConstraint::Type type, const int x,
                         const int y, const char c) {
  QString text;
  switch (type) {//轴平面格式
  case AxisPlaneConstraint::FREE:
    text = QString("FREE (%1)").arg(c);
    break;
  case AxisPlaneConstraint::PLANE:
    text = QString("PLANE (%1)").arg(c);
    break;
  case AxisPlaneConstraint::AXIS:
    text = QString("AXIS (%1)").arg(c);
    break;
  case AxisPlaneConstraint::FORBIDDEN:
    text = QString("FORBIDDEN (%1)").arg(c);
    break;
  }
  drawText(x, y, text);
}

void Viewer::displayDir(const unsigned short dir, const int x, const int y,
                        const char c) {
  QString text;
  switch (dir) {//绘制方向
  case 0:
    text = QString("X (%1)").arg(c);
    break;
  case 1:
    text = QString("Y (%1)").arg(c);
    break;
  case 2:
    text = QString("Z (%1)").arg(c);
    break;
  }
  drawText(x, y, text);
}

void Viewer::displayText() {
  glColor4f(foregroundColor().redF(), foregroundColor().greenF(),
            foregroundColor().blueF(), foregroundColor().alphaF());
  glDisable(GL_LIGHTING);
  drawText(10, height() - 30, "TRANSLATION :");
  displayDir(transDir, 190, height() - 30, 'G');
  displayType(constraints[activeConstraint]->translationConstraintType(), 10,
              height() - 60, 'T');

  drawText(width() - 220, height() - 30, "ROTATION");
  displayDir(rotDir, width() - 100, height() - 30, 'D');
  displayType(constraints[activeConstraint]->rotationConstraintType(),
              width() - 220, height() - 60, 'R');

  switch (activeConstraint) {
  case 0:
    drawText(20, 20, "Constraint direction defined w/r to LOCAL (SPACE)");
    break;
  case 1:
    drawText(20, 20, "Constraint direction defined w/r to WORLD (SPACE)");
    break;
  case 2:
    drawText(20, 20, "Constraint direction defined w/r to CAMERA (SPACE)");
    break;
  }
  glEnable(GL_LIGHTING);
}

QString Viewer::helpString() const {
  QString text("<h2>C o n s t r a i n e d F r a m e</h2>");
  text += "A manipulated frame can be constrained in its displacement.<br><br>";
  text += "Try the different translation (press <b>G</b> and <b>T</b>) and "
          "rotation ";
  text += "(<b>D</b> and <b>R</b>) constraints while moving the frame with the "
          "mouse.<br><br>";
  text += "The constraints can be defined with respect to various coordinates";
  text += "systems : press <b>Space</b> to switch.<br><br>";
  text +=
      "Press the <b>Alt</b> key while moving the mouse to move the camera.<br>";
  text += "Press the <b>Shift</b> key to temporally disable the "
          "constraint.<br><br>";
  text += "You can easily define your own constraints to create a specific "
          "frame behavior.";
  return text;
}
