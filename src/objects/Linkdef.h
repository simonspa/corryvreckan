/**
 * @file
 * @brief Linkdef for ROOT Class generation
 */

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Missing ROOT objects
#pragma link C++ class ROOT::Math::Cartesian2D < unsigned int > +;
#pragma link C++ class ROOT::Math::DisplacementVector2D < ROOT::Math::Cartesian2D < unsigned int >,                         \
    ROOT::Math::DefaultCoordinateSystemTag > +;

// AP2 objects
#pragma link C++ class corryvreckan::Object + ;
#pragma link C++ class corryvreckan::Cluster + ;
#pragma link C++ class corryvreckan::GuiDisplay + ;
#pragma link C++ class corryvreckan::KDTree + ;
#pragma link C++ class corryvreckan::Pixel + ;
#pragma link C++ class corryvreckan::SpidrSignal + ;
#pragma link C++ class corryvreckan::Track + ;
#pragma link C++ class corryvreckan::MCParticle + ;

// Vector of Object for internal storage
#pragma link C++ class std::vector < corryvreckan::Object* > +;
