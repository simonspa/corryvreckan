/**
 * @file
 * @brief Linkdef for ROOT Class generation
 */

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclasses;

// Missing ROOT objects
#pragma link C++ class ROOT::Math::Cartesian2D < unsigned int> + ;
#pragma link C++ class ROOT::Math::DisplacementVector2D < ROOT::Math::Cartesian2D < unsigned int>,                          \
    ROOT::Math::DefaultCoordinateSystemTag> +                                                                               \
    ;

// Corryvreckan objects
#pragma link C++ class corryvreckan::Object + ;
#pragma link C++ class corryvreckan::Pixel + ;
#pragma link C++ class corryvreckan::Cluster + ;
#pragma link C++ class corryvreckan::SpidrSignal + ;
#pragma link C++ class corryvreckan::Track + ;
#pragma link C++ class corryvreckan::StraightLineTrack + ;
#pragma link C++ class corryvreckan::GblTrack + ;
#pragma link C++ class corryvreckan::Multiplet + ;
#pragma link C++ class corryvreckan::MCParticle + ;
#pragma link C++ class corryvreckan::Event + ;
#pragma link C++ class corryvreckan::Track::Plane + ;
#pragma link C++ class corryvreckan::Waveform + ;

#pragma link C++ class corryvreckan::Object::PointerWrapper < corryvreckan::Pixel> + ;
#pragma link C++ class corryvreckan::Object::PointerWrapper < corryvreckan::Cluster> + ;

#pragma link C++ class corryvreckan::Object::BaseWrapper < corryvreckan::Pixel> + ;
#pragma link C++ class corryvreckan::Object::BaseWrapper < corryvreckan::Cluster> + ;

// Vector of Object for internal storage
#pragma link C++ class std::vector < corryvreckan::Object*> + ;
