/* ------------------------------------------------------------------   */
/*      item            : UFODynamics.cxx
        made by         : repa
        from template   : DusimeModuleTemplate.cxx (2022.06)
        date            : Mon Jun 20 17:24:08 2022
        category        : body file
        description     :
        changes         : Mon Jun 20 17:24:08 2022 first version
        language        : C++
        copyright       : (c)
*/


#define UFODynamics_cxx
// include the definition of the module class
#include "UFODynamics.hxx"

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
//#define D_MOD
//#define I_MOD
#include <debug.h>

// class/module name
const char* const UFODynamics::classname = "ufo-dynamics";

// initial condition/trim table
const IncoTable* UFODynamics::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e.
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<_ThisModule_,double>
//       (REF_MEMBER(&_ThisModule_::i_example))}

    // always close off with:
    { NULL, NULL} };

  return inco_table;
}

// parameters to be inserted
const ParameterTable* UFODynamics::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,std::vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "Nonsense dynamics for a UFO."} };

  return parameter_table;
}

// constructor
UFODynamics::UFODynamics(Entity* e, const char* part, const
                       PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be
     called for trim condition calculations, which is normal if you for
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to
     fill a snapshot, or to restore your state from a snapshot. Only
     applicable if you have no state. */
  SimulationModule(e, classname, part, getMyIncoTable(), 12*sizeof(double)),

  // initialize the data you need in your simulation
  body(1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0),
  ws(13),
  tau_r(0.5),
  tau_v(2.0),
  // snapshot data
  snapcopy(),

  // initialize the channel access tokens
  r_controls(getId(), NameSet(getEntity(), "ControlInput", part),
	     "ControlInput", 0, Channel::Continuous, Channel::OnlyOneEntry),
  w_egomotion(getId(), NameSet(getEntity(), "ObjectMotion", part),
	      "BaseObjectMotion", "ufo movement", Channel::Continuous,
	      Channel::OnlyOneEntry),
  // this channel is labeled with our entity name. 
  w_world(getId(), NameSet("world", "BaseObjectMotion", ""),
	  BaseObjectMotion::classname, getEntity(), Channel::Continuous,
	  Channel::OneOrMoreEntries),

  // activity initialization
  // myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "update UFO dynamics", &cb1, ps)
{
  // do the actions you need for the simulation
  // set the UFO at a reasonable height (not in the ice)
  body.initialize(0.0, 0.0, -3.0, 0.0, 0.0, 0.0,
                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  // connect the triggers for simulation
  do_calc.setTrigger(r_controls);
}

bool UFODynamics::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
UFODynamics::~UFODynamics()
{
  //
}

// as an example, the setTimeSpec function
bool UFODynamics::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  // myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool UFODynamics::checkTiming(const std::vector<int>& i)
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
bool UFODynamics::isPrepared()
{
  bool res = true;

  CHECK_TOKEN(r_controls);
  CHECK_TOKEN(w_egomotion);
  CHECK_TOKEN(w_world);

  // return result of checks
  return res;
}

// start the module
void UFODynamics::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void UFODynamics::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// fill a snapshot with state data. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void UFODynamics::fillSnapshot(const TimeSpec& ts,
                               Snapshot& snap, bool from_trim)
{
  // The most efficient way of filling a snapshot is with an AmorphStore
  // object.
  AmorphStore s(snap.accessData(), snap.getDataSize());
  assert(snap.getDataSize() == sizeof(snapcopy));

  // set the right format
  snap.coding = Snapshot::Doubles;

  if (from_trim) {
    // use packData(s, trim_state_variable1); ... to pack your state into
    // the snapshot
  }
  else {
    for (const auto &xs: snapcopy) {
      packData(s, xs);
    }
  }
}

// reload from a snapshot. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void UFODynamics::loadSnapshot(const TimeSpec& ts, const Snapshot& snap)
{
  // access the data in the snapshot with an AmorphReStore object
  AmorphReStore s(snap.accessData(), snap.getDataSize());
  double x(s), y(s), z(s), u(s), v(s), w(s);
  double phi(s), theta(s), psi(s), p(s), q(s), r(s);
  body.initialize(x, y, z, u, v, w, phi, theta, psi, p, q, r);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void UFODynamics::doCalculation(const TimeSpec& ts)
{
  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent: {
    // only repeat the output, do not change the model state

    break;
    }

  case SimulationState::Replay:
  case SimulationState::Advance: {
    // access the input
    try {
      DataReader<ControlInput> u(r_controls, ts);

      // apply forces on the body
      body.zeroForces();
      Vector3 moms { -u.data().roll, -u.data().pitch, -u.data().yaw };
      body.applyBodyMoment((moms - body.X().segment(6,3))/tau_r);
      Vector3 forces { u.data().throttle, 0.0, 0.0 };
      static Vector3 cg {0.0, 0.0, 0.0};
      body.applyBodyForce((forces - body.X().segment(0,3))/tau_v, cg);
    }
    catch(std::exception& e) {
      W_MOD("Could not read control input at " << ts);
    }

    // do the simulation calculations, one step
    integrate_rungekutta(body, ws, ts.getDtInSeconds());

    break;
    }
  default:
    // other states should never be entered for a SimulationModule,
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here,
    throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }

  // calculate additional outputs
  body.output();
  DataWriter<BaseObjectMotion> y(w_egomotion, ts);
  for (unsigned ii = 3; ii--; ) {
    y.data().xyz[ii] = body.X()[3+ii];
    y.data().uvw[ii] = body.X()[ ii];
    y.data().omega[ii] = body.X()[6+ii];
  }
  y.data().setquat(body.phi(), body.theta(), body.psi());

  // copy this, so others see where we are (in multiplayer)
  DataWriter<BaseObjectMotion> pub(w_world, ts);
  pub.data() = y.data();

  if (snapshotNow()) {
    // keep a copy of the current state
    for (unsigned ii = 3; ii--; ) {
      snapcopy[  ii] = body.X()[3+ii]; // copy xyz
      snapcopy[3+ii] = body.X()[  ii]; // uvw
      snapcopy[9+ii] = body.X()[6+ii]; // pqr
    }
    snapcopy[6] = body.phi();
    snapcopy[7] = body.theta();
    snapcopy[8] = body.psi();
  }
}


// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<UFODynamics> a(UFODynamics::getMyParameterTable());

