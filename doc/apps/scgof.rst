*scgof* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It filters origins based on 
their epicentre locations, and keeps only those within a region 
configured with matching creation author. The origin filtering 
procedure is as follows:

#. Listen to origins received from groups defined in `connection.subscriptions`.
#. For each origin, check if author and location matches any of the filters configured with `author`, and an `region`.
#. If the origin checks the previous, transfer origin to the messaging group defined in `connection.primaryGroup`. 

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
