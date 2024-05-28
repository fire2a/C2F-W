import sys
from qgis.core import QgsApplication, QgsProject, QgsCoordinateReferenceSystem, QgsMapRendererCustomPainterJob
from qgis.gui import QgsMapCanvas
from qgis.core import QgsRasterLayer
from qgis.PyQt.QtGui import QImage, QPainter
from qgis.PyQt.QtCore import QSize, Qt

# Set up the QGIS application
QgsApplication.setPrefixPath("/usr/bin/qgis", True)
qgs = QgsApplication([], False)
qgs.initQgis()

# Create a QGIS project instance
project = QgsProject.instance()

gpkg_file = sys.argv[1]
output_file = sys.argv[2]

# Load your GeoPackage layer
layer = QgsRasterLayer(gpkg_file, "fire_scars")

if not layer.isValid() or not layer.crs().isValid():
    print("Layer failed to load or is missing georeferencing information!")
    # Assuming no georeferencing info, set a default CRS (e.g., WGS84)
    default_crs = QgsCoordinateReferenceSystem("EPSG:32718")
    layer.setCrs(default_crs)
    print("Applied default CRS: EPSG:32718")

# Add the layer to the project
project.addMapLayer(layer)

# Set up a map canvas and renderer
map_canvas = QgsMapCanvas()
map_canvas.setLayers([layer])
map_canvas.setExtent(layer.extent())

# Create an image
img = QImage(QSize(800, 600), QImage.Format_ARGB32_Premultiplied)
img.fill(Qt.transparent)

# Set up the painter and render the map
painter = QPainter(img)

map_settings = map_canvas.mapSettings()
map_settings.setOutputSize(img.size())
map_settings.setLayers([layer])
map_settings.setExtent(layer.extent())
map_settings.setOutputSize(QSize(800, 600))
map_settings.setBackgroundColor(Qt.white)

render = QgsMapRendererCustomPainterJob(map_settings, painter)
render.start()
render.waitForFinished()
painter.end()

# Save the image to a file
img.save(output_file, "png")

# Clean up and exit
qgs.exitQgis()
