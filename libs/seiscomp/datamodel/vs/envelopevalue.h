/***************************************************************************
 *   Copyright (C) by ETHZ/SED                                             *
 *                                                                         *
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE as published*
 * by the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This software is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU Affero General Public License for more details.                     *
 *                                                                         *
 *   Developed by gempa GmbH                                               *
 ***************************************************************************/

// This file was created by a source code generator.
// Do not modify the contents. Change the definition and run the generator
// again!


#ifndef __SEISCOMP_DATAMODEL_VS_ENVELOPEVALUE_H__
#define __SEISCOMP_DATAMODEL_VS_ENVELOPEVALUE_H__


#include <seiscomp/datamodel/vs/types.h>
#include <string>
#include <seiscomp/datamodel/object.h>
#include <seiscomp/core/exceptions.h>
#include <seiscomp/datamodel/vs/api.h>


namespace Seiscomp {
namespace DataModel {
namespace VS {


DEFINE_SMARTPOINTER(EnvelopeValue);

class EnvelopeChannel;


class SC_VS_API EnvelopeValue : public Object {
	DECLARE_SC_CLASS(EnvelopeValue);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		EnvelopeValue();

		//! Copy constructor
		EnvelopeValue(const EnvelopeValue& other);

		//! Custom constructor
		EnvelopeValue(double value);
		EnvelopeValue(double value,
		              const std::string& type,
		              const OPT(EnvelopeValueQuality)& quality);

		//! Destructor
		~EnvelopeValue();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		EnvelopeValue& operator=(const EnvelopeValue& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const EnvelopeValue& other) const;
		bool operator!=(const EnvelopeValue& other) const;

		//! Wrapper that calls operator==
		bool equal(const EnvelopeValue& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setValue(double value);
		double value() const;

		void setType(const std::string& type);
		const std::string& type() const;

		void setQuality(const OPT(EnvelopeValueQuality)& quality);
		EnvelopeValueQuality quality() const throw(Seiscomp::Core::ValueException);

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		EnvelopeChannel* envelopeChannel() const;

		//! Implement Object interface
		bool assign(Object* other);
		bool attachTo(PublicObject* parent);
		bool detachFrom(PublicObject* parent);
		bool detach();

		//! Creates a clone
		Object* clone() const;

		void accept(Visitor*);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Attributes
		double _value;
		std::string _type;
		OPT(EnvelopeValueQuality) _quality;
};


}
}
}


#endif
