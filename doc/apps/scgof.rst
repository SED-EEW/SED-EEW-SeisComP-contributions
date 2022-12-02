*scgof* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It filters origins based on 
their epicenter locations, and keeps only those within a region defined
in :ref:`BNA files <sec-gui_layers>`, and configured in a filter 
profile with a matching creation author. The origin filtering procedure is 
as follows:

#. Listen to origins received from groups defined in `connection.subscriptions`.
#. For each origin, check if the author and location match any of the filters configured with `author`, and a `region`.
#. If the origin checks the previous, transfer the origin to the messaging group defined in `connection.primaryGroup`. 

.. note::

 Inappropriate origins are still recorded in the database, they are only 
 discarded for event association.

It requires to setup modules creating the origins to be filtered, with 
a dedicated messaging group, e.g.:

.. code-block:: sh

 connection.primaryGroup = LOCATION_A 

Following the same example, you might add the group "LOCATION_A" to the 
default message groups defined by :ref:`scmaster` in :file:`scmaster.cfg`:

.. code-block:: sh

 msgGroups = LOCATION_A, ...

and let *scgof* listen to the messages to the "LOCATION_A" group. 
Adjust :file:`scgof.cfg`:

.. code-block:: sh

 connection.subscriptions = LOCATION_A
