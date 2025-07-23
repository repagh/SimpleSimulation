/* ------------------------------------------------------------------   */
/*      item            : Compass.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx (2022.06)
        date            : Fri Jan 24 09:09:39 2025
        category        : header file
        description     :
        changes         : Fri Jan 24 09:09:39 2025 first version
        language        : C++
        copyright       : (c)
*/

#ifndef Compass_hxx
#define Compass_hxx

// include the dusime header
#include <dueca.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module
#include <glm/glm.hpp>
#include <boost/scoped_ptr.hpp>

#include <Shader.hxx>
#include <TextRenderer.hxx>
#include <VaoVbo.hxx>

// for now, the gtk4 gl support interferes with OSG viewer ... :-(
#if defined(GL_WITH_GLFW)
#include <extra/gui/glfw/DuecaGLFWWindow.hxx>
typedef dueca::DuecaGLFWWindow DUECAGLWindow;
#elif defined(GL_WITH_X)
#include <extra/gui/X11/BareDuecaGLWindow.hxx>
typedef dueca::BareDuecaGLWindow DUECAGLWindow;
#else
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(4, 0, 0)
#include <extra/gui/gtk4/DuecaGLGtk4Window.hxx>
typedef dueca::DuecaGLGtk4Window DUECAGLWindow;
#elif GTK_CHECK_VERSION(3, 16, 0)
#include <extra/gui/gtk3/DuecaGLGtk3Window.hxx>
typedef dueca::DuecaGLGtk3Window DUECAGLWindow;
#endif
#endif

/** Compass visualization, test with DuecaGLWindow.

    The instructions to create an module of this class from the start
    script are:

    \verbinclude compass.scm
 */
class Compass : public Module, public DUECAGLWindow
{
  /** self-define the module type, to ease writing the parameter table */
  typedef Compass _ThisModule_;

private: // simulation data
  /** Latest heading */
  float heading;

  /** Font for compass */
  std::string font;

  // GL links for triangle
  VaoVbo headingtri;

  // GL links for compass
  VaoVbo compass;

  // simple line shader that
  boost::scoped_ptr<Shader> lineshader;

  // text renderer
  boost::scoped_ptr<TextRenderer> texter;

  /// Current viewport offset
  GLint vp_offx;

  /// Current viewport offset
  GLint vp_offy;

  // Current viewport size
  GLsizei vp_size;

private: // channel access
  ChannelReadToken r_position;

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  // PeriodicAlarm        myclock;

  /** Callback object for simulation calculation. */
  Callback<Compass> cb1;

  /** Activity for simulation calculation. */
  ActivityCallback do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the parameter table. */
  static const ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  Compass(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengthy initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete() final;

  /** Destructor. */
  ~Compass();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int> &i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared() final;

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time) final;

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time) final;

public:
  /** gl draw functions, called to init */
  virtual void initGL() final;

  /** gl draw stuff, called when drawing needed */
  virtual void display() final;

  /** reshape callback, window size changed */
  virtual void reshape(int x, int y) final;

  /** Passive motion test? */
  virtual void passive(int x, int y) final;

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec &ts);
};

#endif
