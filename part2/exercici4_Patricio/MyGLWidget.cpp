#include "MyGLWidget.h"
#include <iostream>

MyGLWidget::MyGLWidget (QWidget* parent) : QOpenGLWidget(parent)
{
  setFocusPolicy(Qt::ClickFocus);  // per rebre events de teclat
  scale = 1.0f;
}

MyGLWidget::~MyGLWidget ()
{
  if (program != NULL)
    delete program;
}

void calculaCapsaContenidora(const Model &patricio,glm::vec3& min,glm::vec3& max)
{
    min.x = max.x = patricio.vertices()[0];
    min.y = max.y = patricio.vertices()[1];
    min.z = max.z = patricio.vertices()[2];
    for(unsigned int i = 3; i < patricio.vertices().size(); i += 3) {
        double x = patricio.vertices()[i];
        double y = patricio.vertices()[i+1];
        double z = patricio.vertices()[i+2];
        if(x < min.x) min.x = x;
        else if (x > max.x) max.x = x;
        if(y < min.y) min.y = y;
        else if (y > max.y) max.y = y;
        if(z < min.z) min.z = z;
        else if (z > max.z) max.z = z;
    }
} 

void MyGLWidget::initializeGL ()
{
  // Cal inicialitzar l'ús de les funcions d'OpenGL
  initializeOpenGLFunctions();  
  patricio.load("./models/Patricio.obj"); 
  scale = 1.0f;
  /*Declaracio atributs dels parametres : MyGLWidget.h
    Al cridar la funcio calCapsaContenidora : dono valor als atributs
  */
  calculaCapsaContenidora(patricio,patrMin,patrMax);
  ra = 1;

  glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (d'esborrat)
  //Activar el z-buffer
  glEnable(GL_DEPTH_TEST);
  //Carrega
  carregaShaders();
  createBuffers();
}

void MyGLWidget::paintPatricio()
{
   float aux = rotate;
   rotate = rotateP;
   modelTransform();
   rotate = aux;
   
   // Activem el VAO per a pintar el Patricio
   glBindVertexArray (VAO_Patricio);
   // pintem
   glDrawArrays (GL_TRIANGLES, 0, patricio.faces().size () * 3);
}

void MyGLWidget::paintTerra()
{

    modelTransform();

    // Activem el VAO per a pintar el terra
    glBindVertexArray (VAO_Terra);

    // pintem
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void MyGLWidget::paintGL () 
{
  //Esborrar el buffer de profunditats a la vegada que el frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  paintPatricio();
  //paintTerra();

  projectTransform();
  // Carreguem la vista
  viewTransform ();

  glBindVertexArray (0);
}
void MyGLWidget::modelTransform () 
{
	//Matriu TG se li aplica 1.rotate 2.scale 3.translate -CBP

    //rota 90º sobre el eix Y (antihorari), es modifica per keyPressEvent
    glm::mat4 transform = glm::rotate(glm::mat4(1.0f),rotate,glm::vec3(0.,1.,0.));  
    //apliquem escalat, es modifica per keyPressEvent
    transform = glm::scale(transform, glm::vec3(scale)); 
    //apliquem translacio a CBP 
    glm::vec3 CBP((patrMin.x + patrMax.x)/2,(patrMin.y + patrMax.y)/2,(patrMin.z + patrMax.z)/2); //formula
    transform = glm::translate(transform,-CBP);
    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}


double calcFOV(double ra, double initialFOV)
{
    if(ra >= 1) return initialFOV;
    else {
    	//calcular angle obertura
        double alpha = initialFOV/2; //perque FOV = 2*alfa
        double newAlpha = atan(tan(alpha)/ra);
        return newAlpha*2;  //retorna FOV
    }
}

void MyGLWidget::projectTransform ()
{
    double ifov= 2*atan((patrMax.y-patrMin.y)/3);
    double fov = calcFOV(ra,ifov);
    glm::mat4 project = glm::perspective(fov,ra,1.5,1.5+(patrMax.z-patrMin.z));
    glUniformMatrix4fv(projLoc,1,GL_FALSE,&project[0][0]);
}

void MyGLWidget::viewTransform ()
{
    //std::cout << "escriure" << endl;
    //lookAt (OBS = 0.0.centre_z , VRP = 0.0.0, UP = 0.1.0 vertical)
    glm::mat4 view = glm::lookAt(glm::vec3(0.,0.,((patrMax.z-patrMin.z)/2 + 1.5)),glm::vec3(0.,0.,0.),glm::vec3(0.,1.,0.));
    glUniformMatrix4fv(viewLoc,1,GL_FALSE,&view[0][0]);
}
void MyGLWidget::resizeGL (int w, int h) 
{
    ra = (double)w/(double)h;
    projectTransform();
    glViewport(0, 0, w, h);
}

void MyGLWidget::keyPressEvent(QKeyEvent* event) 
{
  makeCurrent();
  switch (event->key()) {
    case Qt::Key_S: { // escalar a més gran
      scale += 0.01;
      break;
    }
    case Qt::Key_D: { // escalar a més petit
      scale -= 0.01;
      break;
    }
    case Qt::Key_R: { // rotació
      rotateP += M_PI/20;
	  break;
	}
    default: event->ignore(); break;
  }
  update();
}

void MyGLWidget::createBuffers () 
{
  
  // Creació del Vertex Array Object per pintar
  glGenVertexArrays(1, &VAO_Patricio);
  glBindVertexArray(VAO_Patricio);

  glGenBuffers(1, &VBO_Patricio);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_Patricio);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * patricio.faces().size() * 3 * 3, patricio.VBO_vertices(), GL_STATIC_DRAW);
  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);

  glGenBuffers(1, &VBO_PatricioCol);  //color_Patricio
  glBindBuffer(GL_ARRAY_BUFFER, VBO_PatricioCol);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * patricio.faces().size() * 3 * 3, patricio.VBO_matdiff(), GL_STATIC_DRAW);

  // Activem l'atribut colorLoc
  glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(colorLoc);

  // Dades del terra
  // Dos VBOs, un amb posició i l'altre amb color
  glm::vec3 posterra[4] = {
       glm::vec3(-1.0, -1.0, -1.0),
       glm::vec3(-1.0, -1.0, 1.0),
       glm::vec3(1.0, -1.0, -1.0),
       glm::vec3(1.0, -1.0, 1.0)
  };
  glm::vec3 colterra[4] = {
      glm::vec3(48./255.,63./255.,159./255.),
      glm::vec3(48./255.,63./255.,159./255.),
      glm::vec3(48./255.,63./255.,159./255.),
      glm::vec3(48./255.,63./255.,159./255.),
  };

  // Creació del Vertex Array Object per pintar
  glGenVertexArrays(1, &VAO_Terra);
  glBindVertexArray(VAO_Terra);

  glGenBuffers(1, &VBO_TerraPos);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraPos);
  glBufferData(GL_ARRAY_BUFFER, sizeof(posterra), posterra, GL_STATIC_DRAW);
   
  // Activem l'atribut vertexLoc
  glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vertexLoc);
   
  glGenBuffers(1, &VBO_TerraCol); //color_Terra
  glBindBuffer(GL_ARRAY_BUFFER, VBO_TerraCol);
  glBufferData(GL_ARRAY_BUFFER, sizeof(colterra), colterra, GL_STATIC_DRAW);
   
  // Activem l'atribut colorLoc
  glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(colorLoc);

  glBindVertexArray (0);

}



void MyGLWidget::carregaShaders()
{
  // Creem els shaders per al fragment shader i el vertex shader
  QOpenGLShader fs (QOpenGLShader::Fragment, this);
  QOpenGLShader vs (QOpenGLShader::Vertex, this);
  // Carreguem el codi dels fitxers i els compilem
  fs.compileSourceFile("shaders/fragshad.frag");
  vs.compileSourceFile("shaders/vertshad.vert");
  // Creem el program
  program = new QOpenGLShaderProgram(this);
  // Li afegim els shaders corresponents
  program->addShader(&fs);
  program->addShader(&vs);
  // Linkem el program
  program->link();
  // Indiquem que aquest és el program que volem usar
  program->bind();
  // Obtenim identificador per a l'atribut “vertex” del vertex shader
  vertexLoc = glGetAttribLocation (program->programId(), "vertex");
  // Obtenim identificador per a l'atribut “color” del vertex shader
  colorLoc = glGetAttribLocation (program->programId(), "color");
  // Uniform locations
  transLoc = glGetUniformLocation(program->programId(), "TG");
  projLoc = glGetUniformLocation (program->programId(), "proj");
  viewLoc = glGetUniformLocation (program->programId(), "view");

}
