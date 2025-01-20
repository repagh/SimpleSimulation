/* ------------------------------------------------------------------   */
/*      item            : MonitorTeams.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx (2022.06)
        date            : Mon Oct 30 16:22:37 2023
        category        : body file
        description     :
        changes         : Mon Oct 30 16:22:37 2023 first version
        language        : C++
        copyright       : (c)
*/

#define MonitorTeams_cxx

// include the definition of the module class
#include "MonitorTeams.hxx"

// include additional files needed for your calculation here
#include <dueca/CommObjectWriter.hxx>
#include <dueca/CommObjectReader.hxx>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>
#include <dueca/inter/ReplicatorInfo.hxx>

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

#if GTK_CHECK_VERSION(4, 0, 0)

struct _MyLocation
{
  GObject parent;
  std::string teamname;
  BaseObjectPosition pos;
};

enum MyLocationProperty { MY_XPOS = 1, MY_YPOS, MY_NPROPERTIES };

G_DECLARE_FINAL_TYPE(MyLocation, my_location, MY, LOCATION, GObject);
G_DEFINE_TYPE(MyLocation, my_location, G_TYPE_OBJECT);

static void my_location_set_property(GObject *object, guint property_id,
                                     const GValue *value, GParamSpec *pspec)
{
  // does not work
}

// implemented to get info from list -> interface
static void my_location_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec)
{
  MyLocation *self = MY_LOCATION(object);
  switch (static_cast<MyLocationProperty>(property_id)) {
  case MY_XPOS:
    g_value_set_float(value, self->pos.xyz[0]);
    break;
  case MY_YPOS:
    g_value_set_float(value, self->pos.xyz[1]);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
  }
}

static MyLocation *my_location_new(const std::string &name,
                                   const BaseObjectPosition &pos)
{
  auto res = MY_LOCATION(g_object_new(my_location_get_type(), NULL));
  // rest of the memory is zeroed. Use placement new to properly initialize
  new (&(res->pos)) BaseObjectPosition(pos);
  new (&(res->teamname)) std::string(name);
  return res;
}

static GParamSpec *my_location_properties[MY_NPROPERTIES] = {
  NULL,
  g_param_spec_float("x", "X", "", -1E10, 1E10, 0.0,
                     static_cast<GParamFlags>(G_PARAM_READWRITE |
                                              G_PARAM_EXPLICIT_NOTIFY |
                                              G_PARAM_CONSTRUCT)),
  g_param_spec_float("y", "Y", "", -1E10, 1E10, 0.0,
                     static_cast<GParamFlags>(G_PARAM_READWRITE |
                                              G_PARAM_EXPLICIT_NOTIFY |
                                              G_PARAM_CONSTRUCT))
};

static void my_location_dispose(GObject *object)
{
  auto *self = MY_LOCATION(object);
  // explicit calls of destructors
  self->teamname.std::string::~string();
  self->pos.BaseObjectPosition::~BaseObjectPosition();
}

static void my_location_class_init(MyLocationClass *_klass)
{
  auto klass = G_OBJECT_CLASS(_klass);
  klass->set_property = my_location_set_property;
  klass->get_property = my_location_get_property;
  klass->dispose = my_location_dispose;
  g_object_class_install_properties(klass, G_N_ELEMENTS(my_location_properties),
                                    my_location_properties);
}

static void my_location_init(MyLocation *_self) {}

#endif

// class/module name
const char *const MonitorTeams::classname = "monitor-teams";

// Parameters to be inserted
const ParameterTable *MonitorTeams::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::checkTiming),
      check_timing_description },

    { "position-size",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::setPositionSize),
      "Set the feedback/test window position and size" },

    { "check-announce",
      new MemberCall<_ThisModule_, bool>(&_ThisModule_::checkAnnounce),
      "Check announcement calls" },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "please give a description of this module" }
  };

  return parameter_table;
}

// constructor
MonitorTeams::MonitorTeams(Entity *e, const char *part,
                           const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  r_announce(),
  r_world(getId(), NameSet("world", BaseObjectMotion::classname, part),
          BaseObjectMotion::classname, entry_any, Channel::AnyTimeAspect,
          Channel::ZeroOrMoreEntries),

  // clock
  myclock(),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  cb2(this, &_ThisModule_::doNotify),
  // the module's main activity
  do_calc(getId(), "check up", &cb1, ps),
  do_notify(getId(), "print notification", &cb2, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
}

bool MonitorTeams::checkAnnounce(const bool &c)
{
  if (c) {
    r_announce.reset(new ChannelReadToken(
      getId(), NameSet(getEntity(), ReplicatorInfo::classname, getPart()),
      ReplicatorInfo::classname, 0, Channel::Events));
    do_notify.setTrigger(*r_announce);
  }
  else {
    r_announce.reset();
  }
  return true;
}

bool MonitorTeams::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
#if GTK_CHECK_VERSION(4, 0, 0)

  static GladeCallbackTable cb_links[] = {
    // one button connected as feedback
    { "button_send", "clicked", gtk_callback(&_ThisModule_::cbCollectData) },

    // factory connects to create the label widgets
    { "fact_team", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_xpos", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },
    { "fact_ypos", "setup", gtk_callback(&_ThisModule_::cbSetupLabel) },

    // factory connect to fill the table with data
    { "fact_team", "bind", gtk_callback(&_ThisModule_::cbBindName) },
    { "fact_xpos", "bind", gtk_callback(&_ThisModule_::cbBindProp),
      const_cast<char *>("x") },
    { "fact_ypos", "bind", gtk_callback(&_ThisModule_::cbBindProp),
      const_cast<char *>("y") },
    { NULL }
  };

  window.readGladeFile("../../../monitor-teams/monitor-interface/main.ui",
                       "welcome", this, cb_links);
  // set up a list for the data
  auto teams_view = GTK_COLUMN_VIEW(window["teams"]);
  teams_store = g_list_store_new(my_location_get_type());
  auto selection = gtk_no_selection_new(G_LIST_MODEL(teams_store));
  gtk_column_view_set_model(teams_view, GTK_SELECTION_MODEL(selection));

  UIPalooza values;
  CommObjectReader cor(getclassname<UIPalooza>(), &values);
  window.setValues(cor, "mw_%s", NULL, true);

  window.show();

#endif

  // immediately start the notify activity, will print any information
  do_notify.switchOn();
  return true;
}

// destructor
MonitorTeams::~MonitorTeams() { do_notify.switchOff(); }

// as an example, the setTimeSpec function
bool MonitorTeams::setTimeSpec(const TimeSpec &ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0)
    return false;

  // specify the timespec to the clock
  myclock.changePeriodAndOffset(ts);

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool MonitorTeams::checkTiming(const std::vector<int> &i)
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
bool MonitorTeams::isPrepared()
{
  bool res = true;

  // Check used tokens
  CHECK_TOKEN(r_world);
  if (r_announce)
    CHECK_TOKEN(*r_announce);

  // return result of checks
  return res;
}

// start the module
void MonitorTeams::startModule(const TimeSpec &time) { do_calc.switchOn(time); }

// stop the module
void MonitorTeams::stopModule(const TimeSpec &time) { do_calc.switchOff(time); }

#if GTK_CHECK_VERSION(4, 0, 0)
static guint findIndex(GListModel *list, const std::string &name)
{
  for (auto n = g_list_model_get_n_items(list); n--;) {
    auto tm = MY_LOCATION(g_list_model_get_item(list, n));
    if (tm->teamname == name) {
      return n;
    }
  }
  return 0xffffffff;
}
#endif

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void MonitorTeams::doCalculation(const TimeSpec &ts)
{
  // reading all entries from the channel behind r_world. Note that
  // another option would be to use a ChannelMonitor to watch for changes
  // and use multiple access tokens, each specifically for one entry.
  unsigned ecount = 0;
  r_world.selectFirstEntry();
  while (r_world.haveEntry()) {
    ecount++;
    try {
      DataReader<BaseObjectMotion, MatchIntervalStartOrEarlier> om(r_world);
      std::cout << "Ufo " << r_world.getEntryLabel() << " now at "
                << om.data().xyz << std::endl;
      std::cout << "Current tick " << ts.getValidityStart()
                << ", data generated at " << om.timeSpec().getValidityStart()
                << std::endl;

#if GTK_CHECK_VERSION(4, 0, 0)
      auto idx = findIndex(G_LIST_MODEL(teams_store), r_world.getEntryLabel());
      if (idx == 0xffffffff) {

        // apparently a new team, append to the list, which will result in an
        // updated columnview a real implementation should also look at
        // disappearing teams....
        auto newteam = my_location_new(r_world.getEntryLabel(), om.data());
        g_list_store_append(teams_store, newteam);
        g_object_unref(newteam);
        // gtk_widget_queue_draw(window["teams"]);
      }
      else {

        // known team, a bit coarse, but simply copy the data
        auto team =
          MY_LOCATION(g_list_model_get_item(G_LIST_MODEL(teams_store), idx));
        if (team->pos != om.data()) {
          team->pos = om.data();

          // now notify a change in the properties
          g_object_notify_by_pspec(G_OBJECT(team),
                                   my_location_properties[MY_XPOS]);
          g_object_notify_by_pspec(G_OBJECT(team),
                                   my_location_properties[MY_YPOS]);
        }
      }
#endif
    }
    catch (const NoDataAvailable &e) {
      std::cout << "Ufo " << r_world.getEntryLabel() << " no data" << std::endl;
    }
    r_world.selectNextEntry();
  }

  // This shows we looked.
  std::cout << "There were " << ecount << " entries" << std::endl;
}

void MonitorTeams::doNotify(const TimeSpec &ts)
{
  DataReader<ReplicatorInfo> ri(*r_announce, ts);
  cout << ri.data();
}

#if GTK_CHECK_VERSION(4, 0, 0)
bool MonitorTeams::setPositionSize(const std::vector<int> &pos_size)
{
  return window.setWindow(pos_size);
}

void MonitorTeams::cbSetupLabel(GtkSignalListItemFactory *fact,
                                GtkListItem *item, gpointer user_data)
{
  auto label = gtk_label_new("");
  gtk_list_item_set_child(item, label);
}

void MonitorTeams::cbBindName(GtkSignalListItemFactory *fact, GtkListItem *item,
                              gpointer user_data)
{
  // name does not change, simply setting the string in the label is OK
  auto label = gtk_list_item_get_child(item);
  auto obj = MY_LOCATION(gtk_list_item_get_item(item));
  std::cout << "Setting team name" << obj->teamname << std::endl;
  gtk_label_set_label(GTK_LABEL(label), obj->teamname.c_str());
}

void MonitorTeams::cbBindProp(GtkSignalListItemFactory *fact, GtkListItem *item,
                              gpointer prop)
{
  auto label = gtk_list_item_get_child(item);
  auto obj = MY_LOCATION(gtk_list_item_get_item(item));
  gtk_label_set_label(GTK_LABEL(label), reinterpret_cast<const char *>(prop));
  g_object_bind_property(obj, reinterpret_cast<const char *>(prop), label,
                         "label", G_BINDING_DEFAULT);
}

void MonitorTeams::cbCollectData(GtkWidget *btn, gpointer user_data)
{
  UIPalooza values;
  CommObjectWriter cow(getclassname<UIPalooza>(), &values);
  window.getValues(cow, "mw_%s", NULL, true);
  std::cout << "From interface: " << values << std::endl;
}

#endif

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<MonitorTeams> a(MonitorTeams::getMyParameterTable());
