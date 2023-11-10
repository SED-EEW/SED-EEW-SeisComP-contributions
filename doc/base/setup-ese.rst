.. SETUP:

=========
ESE setup 
=========


.. note::
    
    The following instructions are based on the `SeisComP documentation`_. 


Installation
------------

There are multiple ways to install the ETHZ-SED SeisComP EEW (ESE) package. This is meant to provide the easiest method based on `gsm`_.

.. note::
    
    `gsm`_ is provided by `Gempa`_.


#. Install and enable precise system time (requires superuser privileges), e.g.::

     apt install chrony
     systemctl status chrony


#. Install and configure `gsm`_ according to `gsm repository <https://docs.gempa.de/gsm/>`_.

#. Install SeisComP and ESE:: 

     cd gsm
     ./gsm install seiscomp sed-eew 


Configuration
-------------


#. Configure your SeisComP system according to `post-install configuration instructions <https://www.seiscomp.de/doc/base/tutorials/postinstall.html#configuration>`_. Pay special attention to the configuration of the ``innodb_buffer_pool_size`` (over 2G but no more than 70% of the RAM, more information in https://www.seiscomp.de/doc/base/installation.html#mariadb-mysql)

#. Enable the SeisComP database with EEW extensions::

     mysql -u sysop -p seiscomp < ~/seiscomp/share/db/vs/mysql.sql
     mysql -u sysop -p seiscomp < ~/seiscomp/share/db/wfparam/mysql.sql


#. `Add station inventory <https://www.seiscomp.de/doc/base/tutorials/addstation.html#tutorials-addstation>`_ and  `configure bindings <https://www.seiscomp.de/doc/base/tutorials/processing.html#tutorials-processing>`_ for real-time processing.

#. Configure :ref:`sceewenv`, :ref:`scautopick`, :ref:`scautoloc` and :ref:`scevent` following the recommendation provided in :ref:`scvsmag-configuration`.

#. Enable the required modules::

     seiscomp enable scautopick scautoloc scevent sceewenv scvsmag sceewlog
     seiscomp restart


Test
----

#. Test the ESE setup with `real-time playbacks <https://www.seiscomp.de/doc/base/tutorials/waveformplayback.html#real-time-playbacks>`_ using the data from a recent relevant event is available in the `configured station data archive <https://www.seiscomp.de/doc/base/tutorials/archiving.html>`_.

#. Check the results in ``~/.seiscomp/log/VS_reports/`` (see :ref:`sceewlog-reports`) and :ref:`scolv`.


Further advice
---------------

#. The SeisComP :ref:`fdsnws` module provides a convenient way to access station metadata and event parameters for further analysis. 

#. `Gempa`_ `SMP <http://smp.gempa.de>`_ provides a convenient way to manage a large station metadata inventory.  

#. The `seiscomp shell <https://www.seiscomp.de/doc/base/management.html#seiscomp-shell>`_ provides efficient ways to manage bindings for a large number of stations.

#. The SeisComP ``scgitinit`` utility makes configuration versioning easier. 


References
----------

.. target-notes::

.. _`SeisComP documentation` : https://www.seiscomp.de

.. _`gsm` : https://www.gempa.de/news/2021/07/gsm/

.. _`Gempa` : https://www.gempa.de

