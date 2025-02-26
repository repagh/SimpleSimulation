/* ------------------------------------------------------------------   */
/*      item            : MonitorTeams.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx (2022.06)
        date            : Mon Oct 30 16:22:37 2023
        category        : header file
        description     :
        changes         : Mon Oct 30 16:22:37 2023 first version
        language        : C++
        copyright       : (c)
*/

#ifndef MonitorTeams_hxx
#define MonitorTeams_hxx

// include the dusime header
#include <dueca.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module
#include <gtk/gtk.h>
#include <GtkGladeWindow.hxx>

/** Monitor joining, leaving and updates of ufo vehicles.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude monitor-teams.scm
 */
class MonitorTeams: public Module
{
  /** self-define the module type, to ease writing the parameter table */
  typedef MonitorTeams _ThisModule_;

private: // simulation data
#if GTK_CHECK_VERSION(4, 0, 0)
  // It no longer uses glade, but gtk builder, but the name is still
  // compatible
  GtkGladeWindow window;

  GListStore *teams_store;
#endif

private: // channel access
  /** Read information from the interconnector on joining/leaving peers */
  boost::scoped_ptr<ChannelReadToken>  r_announce;

  /** Read the current state of the peer ufo's flying around */
  ChannelReadToken    r_world;

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm        myclock;

  /** Callback object for simulation calculation. */
  Callback<MonitorTeams>  cb1;

  /** Callback object for simulation calculation. */
  Callback<MonitorTeams>  cb2;

  /** Activity for simulation calculation. */
  ActivityCallback      do_calc;

  /** Activity for simulation calculation. */
  ActivityCallback      do_notify;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  MonitorTeams(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~MonitorTeams();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int>& i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

  /** set reader for interlink announements */
  bool checkAnnounce(const bool& c);

#if GTK_CHECK_VERSION(4, 0, 0)

  /** Set the window position */
  bool setPositionSize(const std::vector<int>& pos_size);

  /** Factory callback for labels in the column view */
  void cbSetupLabel(GtkSignalListItemFactory *fact, GtkListItem *item,
                 gpointer user_data);

  /** Set the name of the team in the 1s column */
  void cbBindName(GtkSignalListItemFactory *fact,
                 GtkListItem *item, gpointer user_data);

  /** Bind moving x or y position to other columns */
  void cbBindProp(GtkSignalListItemFactory *fact,
               GtkListItem *item, gpointer prop);

  /** Demo callback from a button */
  void cbCollectData(GtkWidget* btn, gpointer user_data);
#endif

  /** print a notification about a leaving or joining peer */
  void doNotify(const TimeSpec& ts);
};

#endif
