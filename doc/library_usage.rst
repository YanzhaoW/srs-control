################################################
Library usage
################################################

Use CMake to link the library
#################################

Link the SRS library with CMake `find_package`:

.. code-block:: cmake

  find_package(srs REQUIRED)
  add_executable(main PRIVATE srs::srs)

Please make sure add the directory path of the installation folder ``srs-download`` to ``CMAKE_PREFIX_PATH``:

.. code-block:: bash

  cmake -DCMAKE_PREFIX_PATH=[...]/srs-download ..

Output data structure
################################

The output data structure of this program is a C++ struct:

.. doxygenstruct:: srs::StructData
   :project: srs
   :members:
   :undoc-members:

with its sub structure:

.. doxygenstruct:: srs::HitData
   :project: srs
   :members:


.. doxygenstruct:: srs::MarkerData
   :project: srs
   :members:


.. doxygenstruct:: srs::ReceiveDataHeader
   :project: srs
   :members:

An example of ROOT macro to extract data structure from the tree
=================================================================

To extract the data structure value, it's highly recommended to use ROOT `TTreeReader <https://root.cern/doc/master/classTTreeReader.html>`_ instead of direct ROOT ``TTree``. The tree name from the :ref:`srs-control` save a tree named "srs_data_tree" with a single branch named "srs_frame_data".

.. attention::
   To let ROOT use the dictionary of ``srs::StructData``, the libaray path ``srs-download/lib`` has to be added to ``LD_LIBRARY_PATH`` before running the Macro file: ``export LD_LIBRARY_PATH="[.../srs-download]/lib:$LD_LIBRARY_PATH"``

The following example extracts the whole data structure :cpp:type:`srs::StructData` and print out every offset value:

.. code:: cpp

   int check_srs()
   {
       auto file = std::make_unique<TFile>("output.root", "read");
       auto tree_reader = TTreeReader{ "srs_data_tree", file.get() };

       auto srs_struct_data = TTreeReaderValue<srs::StructData>{ tree_reader, "srs_frame_data" };

       while (tree_reader.Next())
       {
           const auto& hit_data = srs_struct_data->hit_data;
           for (const auto& hit : hit_data)
           {
               std::cout << "offset: " << static_cast<int>(hit.offset) << "\n";
           }
       }
       return 0;
   }

Available APIs
##################################

.. toctree::
   :maxdepth: 1

   stubs/library_apis
