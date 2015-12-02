/************************************************************/
/*    AUTH: Simon Rohou
/*    FILE: Tube.cpp
/*    PRJT: TubeIbex https://github.com/SimonRohou/TubeIbex
/*    DATE: 2015
/************************************************************/

#include "Tube.h"
#include <iomanip> // for setprecision()

using namespace std;
using namespace ibex;

Tube::Tube(const Interval &intv_t, unsigned int slices_number)
{
  if((slices_number == 0) || (slices_number & (slices_number - 1))) // decrement and compare
    cout << "Warning Tube::Tube(): slices number (" << slices_number << ") not a power of 2." << endl;

  m_intv_t = intv_t;
  m_intv_y = Interval::EMPTY_SET;
  m_slices_number = slices_number;

  if(slices_number == 1)
  {
    m_first_subtube = NULL;
    m_second_subtube = NULL;
  }

  else
  {
    pair<Interval,Interval> bisection = intv_t.bisect(0.5);
    m_first_subtube = new Tube(bisection.first, slices_number / 2);
    m_second_subtube = new Tube(bisection.second, slices_number / 2);
  }

  update();
}

Tube::Tube(const Tube& tu)
{
  m_intv_t = tu.getT();
  m_intv_y = tu.getY();
  m_slices_number = tu.size();
  m_enclosed_bounds = tu.getEnclosedBounds();
  m_intv_integral = tu.integral();

  if(tu.isSlice())
  {
    m_first_subtube = NULL;
    m_second_subtube = NULL;
  }

  else
  {
    m_first_subtube = new Tube(*(tu.getFirstSubTube()));
    m_second_subtube = new Tube(*(tu.getSecondSubTube()));
  }
}

Tube::~Tube()
{
  delete m_first_subtube;
  delete m_second_subtube;
}

void Tube::resample(int new_slices_number)
{
  if((new_slices_number == 0) || (new_slices_number & (new_slices_number - 1))) // decrement and compare
    cout << "Warning Tube::Tube(): slices number (" << new_slices_number << ") not a power of 2." << endl;

  if(isSlice())
  {
    pair<Interval,Interval> bisection = m_intv_t.bisect(0.5);
    m_first_subtube = new Tube(bisection.first, new_slices_number / 2);
    m_first_subtube->setY(m_intv_y);
    m_second_subtube = new Tube(bisection.second, new_slices_number / 2);
    m_second_subtube->setY(m_intv_y);
  }

  else
  {
    m_first_subtube->resample(new_slices_number / 2);
    m_second_subtube->resample(new_slices_number / 2);
  }

  m_slices_number = new_slices_number;
  update();
}

int Tube::size() const
{
  return m_slices_number;
}

bool Tube::isSlice() const
{
  return m_first_subtube == NULL && m_second_subtube == NULL;
}

Tube* Tube::getSlice(int index)
{
  if(index < 0 || index >= m_slices_number)
  {
    cout << "Error Tube::getSlice(int): out of range "
         << "for index=" << index << " in [0," << m_slices_number << "[" << endl;
    return NULL;
  }

  else if(isSlice())
  {
    if(index != 0)
      cout << "Warning Tube::getX(int): index not null in slice." << endl;

    return this;
  }

  else
  {
    if(index >= size() / 2)
      return m_second_subtube->getSlice(index - (size() / 2));

    else
      return m_first_subtube->getSlice(index);
  }
}

double Tube::getSliceWidth() const
{
  if(isSlice())
    return m_intv_t.diam();

  else
    return m_first_subtube->getSliceWidth();
}

int Tube::input2index(double t) const
{
  if(!m_intv_t.contains(t))
  {
    cout << "Error Tube::time2index(double): no corresponding slice "
         << "for t=" << t << " in " << setprecision(16) << m_intv_t << endl;
    return -1;
  }

  return (int)(m_slices_number * (t - m_intv_t.lb()) / m_intv_t.diam());
}

double Tube::index2input(int index) const
{
  if(index < 0 || index >= m_slices_number)
  {
    cout << "Error Tube::index2time(int): out of range "
         << "for index=" << index << " in [0," << m_slices_number << "[" << endl;
    return -1;
  }

  return 1.5 * index * (m_intv_t.ub() - m_intv_t.lb()) / m_slices_number;
}

const Interval& Tube::getT() const
{
  return m_intv_t;
}

const Interval& Tube::getT(int index)
{
  return getSlice(index)->getT();
}

const Interval& Tube::getT(double t)
{
  return getSlice(input2index(t))->getT();
}

const Interval& Tube::getY(int index)
{
  return getSlice(index)->m_intv_y;
}

const Interval& Tube::getY(double t)
{
  return getSlice(input2index(t))->m_intv_y;
}

Interval Tube::getY(const Interval& intv_t) const
{
  if(!m_intv_t.intersects(intv_t))
    return Interval::EMPTY_SET;

  else if(isSlice() || intv_t.is_unbounded() || intv_t.is_superset(m_intv_t))
    return m_intv_y;

  else
    return m_first_subtube->getY(intv_t) | m_second_subtube->getY(intv_t);
}

void Tube::setY(const Interval& intv_y, int index)
{
  getSlice(index)->setY(intv_y);
  updateFromIndex(index);
}

void Tube::setY(const Interval& intv_y, double t)
{
  getSlice(input2index(t))->setY(intv_y);
  updateFromInput(t);
}

void Tube::setY(const Interval& intv_y, const Interval& intv_t)
{
  if(m_intv_t.intersects(intv_t))
  {
    if(isSlice())
      m_intv_y = intv_y;

    else
    {
      m_first_subtube->setY(intv_y, intv_t);
      m_second_subtube->setY(intv_y, intv_t);
    }
    
    update();
  }
}

const Interval Tube::integral(const Interval& intv_t) const
{
  if(!m_intv_t.intersects(intv_t) || (m_intv_t & intv_t).diam() == 0.)
    return Interval(0.);

  else if(isSlice())
    return m_intv_y * (m_intv_t & intv_t).diam();

  else
    return m_first_subtube->integral(intv_t) + m_second_subtube->integral(intv_t);
}

const pair<Interval,Interval> Tube::integral(const Interval& intv_t1, const Interval& intv_t2) const
{
  return make_pair(integral(Interval(intv_t1.ub(), intv_t2.lb())),
                   integral(Interval(intv_t1.lb(), intv_t2.ub())));
}

const Interval Tube::integralIntervalBounds(const Interval& intv_t1, const Interval& intv_t2) const
{
  return Interval(integral(Interval(intv_t1.ub(), intv_t2.lb())).lb(),
                        integral(Interval(intv_t1.lb(), intv_t2.ub())).ub());
}

bool Tube::intersect(const Interval& intv_y, int index)
{
  bool result = getSlice(index)->intersect(intv_y, Interval::ALL_REALS, false);
  updateFromIndex(index);
  return result;
}

bool Tube::intersect(const Interval& intv_y, double t)
{
  bool result = getSlice(input2index(t))->intersect(intv_y, Interval::ALL_REALS, false);
  updateFromInput(t);
  return result;
}

bool Tube::intersect(const Interval& intv_y, const Interval& intv_t, bool allow_update)
{
  if(m_intv_t.intersects(intv_t))
  {
    bool contraction;

    if(isSlice())
    {
      double diam = m_intv_y.diam();
      m_intv_y &= intv_y;
      contraction = m_intv_y.diam() < diam;
    }

    else
    {
      contraction |= m_first_subtube->intersect(intv_y, intv_t, false);
      contraction |= m_second_subtube->intersect(intv_y, intv_t, false);
    }
    
    if(contraction && allow_update)
      update();

    return contraction;
  }

  return false;
}

const pair<Interval,Interval> Tube::getEnclosedBounds() const
{
  return m_enclosed_bounds;
}

const pair<Interval,Interval> Tube::getEnclosedBounds(const Interval& intv_t) const
{
  if(!intv_t.is_empty() && m_intv_t.intersects(intv_t))
  {
    if(isSlice() || intv_t.is_superset(m_intv_t))
      return m_enclosed_bounds;

    else
    {
      pair<Interval,Interval> ui_past = m_first_subtube->getEnclosedBounds(intv_t);
      pair<Interval,Interval> ui_future = m_second_subtube->getEnclosedBounds(intv_t);
      return make_pair(ui_past.first | ui_future.first, ui_past.second | ui_future.second);
    }
  }

  return make_pair(Interval::EMPTY_SET, Interval::EMPTY_SET);
}

void Tube::print() const
{
  if(isSlice())
    cout << "Tube: " << m_intv_t << " \t" << m_intv_y << endl;

  else
  {
    m_first_subtube->print();
    m_second_subtube->print();
  }
}

std::ostream& operator<<(std::ostream& os, const Tube& x)
{ 
  cout << "Tube: t=" << x.m_intv_t 
       << ", y=" << x.m_intv_y 
       << ", slices=" << x.m_slices_number
       << flush;
  return os;
}

Tube& Tube::operator |=(const Tube& x)
{
  if(size() != x.size())
    cout << "Warning Tube::operator |=: cannot make the hull of Tubes of different dimensions: " 
         << "n1=" << size() << " and n2=" << x.size() << endl;

  if(getT() != x.getT())
    cout << "Warning Tube::operator |=: cannot make the hull of Tubes of different domain: " 
         << "[t1]=" << getT() << " and [t2]=" << x.getT() << endl;

  m_intv_y |= x.getY();

  if(!isSlice())
  {
    *m_first_subtube |= *(x.getFirstSubTube());
    *m_second_subtube |= *(x.getSecondSubTube());
  }

  update();
  
  return *this;
}

Tube& Tube::operator &=(const Tube& x)
{
  if(size() != x.size())
    cout << "Warning Tube::operator &=: cannot make the intersection of Tubes of different dimensions: " 
         << "n1=" << size() << " and n2=" << x.size() << endl;

  if(getT() != x.getT())
    cout << "Warning Tube::operator &=: cannot make the intersection of Tubes of different domain: " 
         << "[t1]=" << getT() << " and [t2]=" << x.getT() << endl;

  m_intv_y &= x.getY();

  if(!isSlice())
  {
    *m_first_subtube &= *(x.getFirstSubTube());
    *m_second_subtube &= *(x.getSecondSubTube());
  }

  update();
  
  return *this;
}

const Tube* Tube::getFirstSubTube() const
{
  return m_first_subtube;
}

const Tube* Tube::getSecondSubTube() const
{
  return m_second_subtube;
}

void Tube::update()
{
  updateFromIndex(-1);
}

void Tube::updateFromInput(double t_focus)
{
  updateFromIndex(input2index(t_focus));
}

void Tube::updateFromIndex(int index_focus)
{
  if(index_focus == -1 || index_focus < m_slices_number)
  {
    if(isSlice())
    {
      m_enclosed_bounds = make_pair(Interval(m_intv_y.lb()), Interval(m_intv_y.ub()));
      m_intv_integral = m_intv_y * m_intv_t.diam();
    }

    else
    {
      if(index_focus < m_slices_number / 2)
        m_first_subtube->updateFromIndex(index_focus);
      
      else
        m_second_subtube->updateFromIndex(index_focus == -1 ? -1 : index_focus - (size() / 2));
      
      m_intv_y = m_first_subtube->getY() | m_second_subtube->getY();
      m_intv_integral = m_first_subtube->m_intv_integral + m_second_subtube->m_intv_integral;

      pair<Interval,Interval> ui_past = m_first_subtube->getEnclosedBounds();
      pair<Interval,Interval> ui_future = m_second_subtube->getEnclosedBounds();
      m_enclosed_bounds = make_pair(ui_past.first | ui_future.first, ui_past.second | ui_future.second);
    }
  }
}