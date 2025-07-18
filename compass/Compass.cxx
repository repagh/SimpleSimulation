/* ------------------------------------------------------------------   */
/*      item            : Compass.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx (2022.06)
        date            : Fri Jan 24 09:09:39 2025
        category        : body file
        description     :
        changes         : Fri Jan 24 09:09:39 2025 first version
        language        : C++
        copyright       : (c)
*/

// freetype font rendering
// https://learnopengl.com/In-Practice/Text-Rendering
// https://open.gl/drawing

#define Compass_cxx

// include the definition of the module class
#include "Compass.hxx"

// include additional files needed for your calculation here
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define DO_INSTANTIATE
#include <dueca/dueca.h>

// This is a DUECA-typical debug printer,
#define DEBPRINTLEVEL 1
#include <debprint.h>

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// #include <GLFW/glfw3.h>
#include <epoxy/glx.h>

// simple, almost hard-wired triangle. If this does not work,
// look elsewhere
#define TESTIMAGE 0

// class/module name
const char *const Compass::classname = "compass";

// Parameters to be inserted
const ParameterTable *Compass::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::checkTiming),
      check_timing_description },

    { "compass-font",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::font),
      "Truetype font to be used in the compass." },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "Demo GL window." }
  };

  return parameter_table;
}

// constructor
Compass::Compass(Entity *e, const char *part, const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),
  DUECAGLWindow("compass", true, false, false),

  // initialize the data you need in your simulation or process
  heading(0.0f),
  font("/usr/share/fonts/open-sans/OpenSans-Regular.ttf"),

  // initialize the channel access tokens, check the documentation for the
  r_position(getId(), NameSet(getEntity(), "ObjectMotion", part),
             getclassname<BaseObjectPosition>(), 0),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  // the module's main activity
  do_calc(getId(), "update compass view", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(r_position);
}

bool Compass::complete()
{
  // open the window
  openWindow();

  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
Compass::~Compass()
{
  // make my context active again, because GL objects are going
  // to be freed. When the DuecaGLWindow is destroyed, the GL
  // context itself will be released
  selectGraphicsContext();
  texter.reset();
  lineshader.reset();
}

// as an example, the setTimeSpec function
bool Compass::setTimeSpec(const TimeSpec &ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0)
    return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool Compass::checkTiming(const std::vector<int> &i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool Compass::isPrepared()
{
  bool res = true;

  // Example checking a token:
  CHECK_TOKEN(r_position);

  // return result of checks
  return res;
}

// start the module
void Compass::startModule(const TimeSpec &time) { do_calc.switchOn(time); }

// stop the module
void Compass::stopModule(const TimeSpec &time) { do_calc.switchOff(time); }

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void Compass::doCalculation(const TimeSpec &ts)
{
  try {
    // read latest heading
    DataReader<BaseObjectPosition> p(r_position, ts);
    heading = p.data().getPsi();
  }
  catch (const dueca::NoDataAvailable &e) {
    // might happen
  }

  // ask for a redraw
  redraw();
}

GLenum glCheckError_(const char *file, int line)
{
  GLenum errorCode;
  while ((errorCode = glGetError()) != GL_NO_ERROR) {
    std::string error;
    switch (errorCode) {
    case GL_INVALID_ENUM:
      error = "INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      error = "INVALID_VALUE";
      break;
    case GL_INVALID_OPERATION:
      error = "INVALID_OPERATION";
      break;
    case GL_STACK_OVERFLOW:
      error = "STACK_OVERFLOW";
      break;
    case GL_STACK_UNDERFLOW:
      error = "STACK_UNDERFLOW";
      break;
    case GL_OUT_OF_MEMORY:
      error = "OUT_OF_MEMORY";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      error = "INVALID_FRAMEBUFFER_OPERATION";
      break;
    }
    std::cout << error << " | " << file << " (" << line << ")" << std::endl;
  }
  return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

#if TESTIMAGE
static GLuint vao = 0;
static const char *shaderCodeVertex = R"(
#version 460 core
layout (location=0) out vec3 color;
const vec2 pos[3] = vec2[3](
	vec2(-0.6, -0.4),
	vec2( 0.6, -0.4),
	vec2( 0.0,  0.6)
);
const vec3 col[3] = vec3[3](
	vec3( 1.0, 0.0, 0.0 ),
	vec3( 0.0, 1.0, 0.0 ),
	vec3( 0.0, 0.0, 1.0 )
);
void main()
{
	gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
	color = col[gl_VertexID];
}
)";

static const char *shaderCodeFragment = R"(
#version 460 core
layout (location=0) in vec3 color;
layout (location=0) out vec4 out_FragColor;
void main()
{
	out_FragColor = vec4(color, 1.0);
};
)";
static GLuint shaderVertex;
static GLuint shaderFragment;
static GLuint program;
#endif

void Compass::initGL()
{
  DEB("InitGL");
  // is GL loaded? should be according to doc??
#if TESTIMAGE
  shaderVertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shaderVertex, 1, &shaderCodeVertex, nullptr);
  glCheckError();
  glCompileShader(shaderVertex);
  glCheckError();

  shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shaderFragment, 1, &shaderCodeFragment, nullptr);
  glCheckError();
  glCompileShader(shaderFragment);
  glCheckError();

  program = glCreateProgram();
  glAttachShader(program, shaderVertex);
  glCheckError();
  glAttachShader(program, shaderFragment);
  glCheckError();
  glLinkProgram(program);
  glCheckError();
  glUseProgram(program);
  glCheckError();

  glCreateVertexArrays(1, &vao);
  glCheckError();
  glBindVertexArray(vao);
  glCheckError();

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glCheckError();

#else
  // once and only once
  if (!lineshader) {

    // default opengl state for this project
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // shader for the lines
    lineshader.reset(new Shader("../../../compass/lines_vrt.glsl",
                                "../../../compass/lines_frg.glsl"));

    // ac triangle
    float headsuptri[] = { 0.0, 0.5, 0.2, -0.4, -0.2, -0.4 };

    // initialize the buffer and array objects
    headingtri.init();

    // set the triangle vertices
    glBufferData(GL_ARRAY_BUFFER, sizeof(headsuptri), headsuptri,
                 GL_STATIC_DRAW);

    // explains the set-up, each vertex is two floats, tightly packed
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);

    // enable this one
    glEnableVertexAttribArray(0);
    headingtri.unbind();

    // data for the compass
    // per 10 deg, with a larger line each 30
    vector<float> compassv;
    compassv.reserve(144);
    for (unsigned ii = 360; ii; ii -= 10) {
      compassv.push_back(0.98 * sin(ii / 180.0 * M_PI));
      compassv.push_back(0.98 * cos(ii / 180.0 * M_PI));
      if (ii % 30 == 0) {
        compassv.push_back(0.8 * sin(ii / 180.0 * M_PI));
        compassv.push_back(0.8 * cos(ii / 180.0 * M_PI));
      }
      else {
        compassv.push_back(0.9 * sin(ii / 180.0 * M_PI));
        compassv.push_back(0.9 * cos(ii / 180.0 * M_PI));
      }
    }

    // tie to vao/vbo pair
    compass.init();
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * compassv.size(),
                 compassv.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);
    compass.unbind();

    // create a new text renderer
    texter.reset(new TextRenderer(font.c_str()));
  }
#endif
}

void Compass::display()
{
#if TESTIMAGE
  glViewport(0, 0, getWidth(), getHeight());
  glClear(GL_COLOR_BUFFER_BIT);
  glCheckError();
  glUseProgram(program);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glCheckError();
#else

  DEB1("Display called");

  // background color
  glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // projection matrix, start with identity
  glm::mat4 projection = glm::mat4(1.0f);
  auto line_proj = lineshader->getLink("projection", projection);

  // fixed head-up triangle
  lineshader->use();
  line_proj.update(projection);

  // draws the heading triangle
  glBindVertexArray(headingtri.vao);
  glDrawArrays(GL_LINE_LOOP, 0, 3);

  // rotate the projection matrix for the compass rose
  projection = glm::rotate(projection, heading, glm::vec3(0.0f, 0.0f, 1.0f));
  line_proj.update(projection);

  // draws the compass lines
  glBindVertexArray(compass.vao);
  glDrawArrays(GL_LINES, 0, 72);

  // N(orth)
  texter->setProjection(projection);
  glm::vec3 green = { 0.0f, 1.0f, 0.0f };
  texter->renderText("N", -0.08, 0.6, 0.005, green);

  // shift by 90 degrees
  projection = glm::rotate(projection, M_PI_2f, glm::vec3(0.0f, 0.0f, 1.0f));
  texter->setProjection(projection);
  texter->renderText("W", -0.1, 0.6, 0.005, green);

  // again, for south
  projection = glm::rotate(projection, M_PI_2f, glm::vec3(0.0f, 0.0f, 1.0f));
  texter->setProjection(projection);
  texter->renderText("S", -0.08, 0.6, 0.005, green);

  // last one east
  projection = glm::rotate(projection, M_PI_2f, glm::vec3(0.0f, 0.0f, 1.0f));
  texter->setProjection(projection);
  texter->renderText("E", -0.08, 0.6, 0.005, green);
#endif

  // release array binding
  glBindVertexArray(0);
}

void Compass::reshape(int x, int y)
{
  DEB("Reshape to " << x << "," << y);
  if (x > y) {
    glViewport((x - y) / 2, 0, y, y);
  }
  else {
    glViewport(0, (y - x) / 2, x, x);
  }
}

void Compass::passive(int x, int y)
{
  DEB("Mouse over " << x << ", " << y);
}
// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<Compass> a(Compass::getMyParameterTable());
