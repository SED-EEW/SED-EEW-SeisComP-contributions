*sceewdump* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It dumps EEW parameters from a given 
event ID or file.

.. note::

 *sceewdump* is a utility of :ref:`sceewlog` and intends to follow the same logic 
 while selecting preferred EEW solutions. However, the preference order for authors 
 is the only criterion currently implemented.

Example (setup databse parameter in :file:`sceewdump.cfg`):

.. code-block:: sh

 sceewdump -E smi:ch.ethz.sed/sc20d/Event/2022njvrzz  -i ~/playback_fm4test/2022njvrzz.xml --xsl /usr/local/share/sceewlog/sc3ml_0.12__quakeml_1.2-RT_eewd.xsl -a scvs,scfdfo
