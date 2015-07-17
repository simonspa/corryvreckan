#include <vector>

#include "SciFiCluster.h"

SciFiCluster::~SciFiCluster(){}

SciFiCluster::SciFiCluster()
: chans()
, adcs()
, clusterSize(0)
, m_ADC(0)
, m_position(0)
{
  chans.clear();
  adcs.clear();
}

void SciFiCluster::addHit(int chan, double adc)
{
  chans.push_back(chan);
  adcs.push_back(adc);
  clusterSize++;
	m_ADC+=adc;
}
