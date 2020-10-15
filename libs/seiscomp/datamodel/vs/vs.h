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


#ifndef SEISCOMP_DATAMODEL_VS_VS_H
#define SEISCOMP_DATAMODEL_VS_VS_H


#include <vector>
#include <seiscomp/datamodel/notifier.h>
#include <seiscomp/datamodel/publicobject.h>
#include <seiscomp/core/exceptions.h>
#include <seiscomp/datamodel/vs/api.h>


namespace Seiscomp {
namespace DataModel {
namespace VS {


DEFINE_SMARTPOINTER(VS);
DEFINE_SMARTPOINTER(Envelope);


class SC_VS_API VS : public PublicObject {
	DECLARE_SC_CLASS(VS);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		VS();

		//! Copy constructor
		VS(const VS& other);

		//! Destructor
		~VS();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		//! No changes regarding child objects are made
		VS& operator=(const VS& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const VS& other) const;
		bool operator!=(const VS& other) const;

		//! Wrapper that calls operator==
		bool equal(const VS& other) const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		/**
		 * Add an object.
		 * @param obj The object pointer
		 * @return true The object has been added
		 * @return false The object has not been added
		 *               because it already exists in the list
		 *               or it already has another parent
		 */
		bool add(Envelope* obj);

		/**
		 * Removes an object.
		 * @param obj The object pointer
		 * @return true The object has been removed
		 * @return false The object has not been removed
		 *               because it does not exist in the list
		 */
		bool remove(Envelope* obj);

		/**
		 * Removes an object of a particular class.
		 * @param i The object index
		 * @return true The object has been removed
		 * @return false The index is out of bounds
		 */
		bool removeEnvelope(size_t i);

		//! Retrieve the number of objects of a particular class
		size_t envelopeCount() const;

		//! Index access
		//! @return The object at index i
		Envelope* envelope(size_t i) const;

		//! Find an object by its unique attribute(s)
		Envelope* findEnvelope(const std::string& publicID) const;

		//! Implement Object interface
		bool assign(Object* other);
		bool attachTo(PublicObject* parent);
		bool detachFrom(PublicObject* parent);
		bool detach();

		//! Creates a clone
		Object* clone() const;

		//! Implement PublicObject interface
		bool updateChild(Object* child);

		void accept(Visitor*);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Aggregations
		std::vector<EnvelopePtr> _envelopes;

	DECLARE_SC_CLASSFACTORY_FRIEND(VS);
};


}
}
}


#endif
