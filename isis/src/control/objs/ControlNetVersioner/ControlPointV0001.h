#ifndef ControlPointV0001_h
#define ControlPointV0001_h
/**
 * @file
 * $Revision: 1.9 $
 * $Date: 2009/07/15 17:33:52 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

 #include <QSharedPointer>

 #include "ControlNetFileProtoV0001.pb.h"

namespace Isis {
  class Pvl;

  class ControlPointV0001 {
    public:
      ControlPointV0001(const Pvl &pointObject);
      ControlPointV0001(const ControlPointV0001 &oldPoint);

      QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> pointData();

    private:
      // These are intentionally not implemented
      ControlPointV0001();
      ControlPointV0001(const &ControlPointV0001 other);
      ControlPointV0001 &operator=(const &ControlPointV0001 other);

      // methods for converting from Pvl to protobuf
      void copy(PvlContainer &container,
                QString keyName,
                QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> point,
                void (ControlNetFileProtoV0001_PBControlPoint::*setter)(bool));
      void copy(PvlContainer &container,
                QString keyName,
                QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> &point,
                void (ControlNetFileProtoV0001_PBControlPoint::*setter)(double));
      void copy(PvlContainer &container,
                QString keyName,
                QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> &point,
                void (ControlNetFileProtoV0001_PBControlPoint::*setter)(const std::string&));
      void copy(PvlContainer &container,
                QString keyName,
                ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure &measure,
                void (ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::*setter)(bool));
      void copy(PvlContainer &container,
                QString keyName,
                ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure &measure,
                void (ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::*setter)(double));
      void copy(PvlContainer &container,
                QString keyName,
                ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure &measure,
                void (ControlNetFileProtoV0001_PBControlPoint::PBControlMeasure::*setter)(const std::string &));

      QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> m_pointData;
      /**< protobuf container that holds information used to create a control point.*/
  };
}

#endif