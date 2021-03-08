============
  Glossary
============

.. glossary::
   :sorted:

   Display : Pixel Management
      Describes the display colorimetry that a particular window / GUI component is being displayed on.

   View : Pixel Management
      A particular rendering of the internal data or colour representation, designed for the display chosen above.

   Look : Pixel Management
      A creative / aesthetic twist on the view above.

   Conform : Editorial / Post Production
      The process of collecting, assembling, and ordering images from a list.

   Offline : Editorial / Post Production
      In editorial / post production terminology, a lower quality version of the internal data that reasonably approximates how the data will look when rendered for final. Trade offs are made to accommodate realtime interaction needs over highest possible quality. This is opposite to the terminology laid out by rendering engines, where offline indicates highest, non-realtime quality.

   Online : Editorial / Post Production
      In editorial / post production terminology, the highest possible quality version of the internal data. Trade offs are made to accommodate the highest possible quality over realtime design needs. This is opposite to the terminology laid out by rendering engines, where online indicates a lower quality, realtime quality approximation.

   Directed Acyclic Graph (DAG) : Internal Software Architecture
      TODO

   Edge : Directed Acyclic Graph (DAG)
      Edges describe the flows between Vertices in the DAG

   Vertex : Directed Acyclic Graph (DAG)
      A single entity at the software model level.

   Panel : Graphical User Interface
      A specific representation of the internal software model data, according to a specific need.

   Flow Panel : Graphical User Interface
      Audience-facing graphical representation of relevant attributes of the internal DAG structure. Contains all lower-level exposed DAG elements. Primary view purpose is to reveal a topographical high-level view of flows between software entities.

   Pipe : Flow Panel
      A UI element in the Flow View which shows how information flows between two Nodes

   Node : Flow Panel
      A UI element in the Flow View which represents a singular entity.

   Timeline Panel : Graphical User Interface
      Audience-facing graphical representation of relevant DAG vertices. Contains Tracks, which contain Clips. Primary view purpose is to reveal temporal relationships between Clips and Tracks.

   Frame : Timeline Panel
      The basic, lowest level visual unit. An image.

   Clip : Timeline Panel
      Graphical representation of an interactive element in the Timeline. It can be manipulated along time left and right, as well as changed in length, and other such interactive needs. Represents either a series of Frames or a more abstract placeholder.

   Gap : Timeline Panel
      A unique type of Clip-like positional holder that represents empty space.

   Track : Timeline Panel
      Graphical representation of a reel or channel upon which Clips can be placed and manipulated.

   Display Panel : Graphical User Interface
      Visual output of imagery.
