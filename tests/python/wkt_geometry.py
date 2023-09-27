"""
Using geopandas to read a GeoJSON file and convert it to a WKT geometry.
"""
import os
import geopandas as gpd
import shapely

def read_geojson_to_wkt(geojson_file):
    """
    Read a GeoJSON file and convert it to a WKT geometry.
    """
    gdf = gpd.read_file(geojson_file)
    return gdf.geometry.to_wkt()

def save_geojson_to_csv(geojson_file, csv_file, is_wkb=True):
    """
    Read a GeoJSON file and save it to a CSV file.
    """
    gdf = gpd.read_file(geojson_file)
    if is_wkb:
        # create a new column with the WKB representation of the geometry
        gdf['geom'] = gdf.geometry.to_wkb(hex=True)
        # remove the old geometry column
        gdf.drop('geometry', axis=1, inplace=True)
    gdf.to_csv(csv_file, index=False)

def save_geojson_to_arrow(geojson_file, arrow_file):
    """
    Read a GeoJSON file and save it to an Arrow file.
    """
    gdf = gpd.read_file(geojson_file)
    gdf.to_feather(arrow_file)

def load_csv_to_geopandas(csv_file, is_wkb=True):
    """
    Load a CSV file into a GeoPandas dataframe.
    """
    if is_wkb:
        # read the WKB column as a bytes array
        gdf = gpd.read_csv(csv_file, converters={'geom': lambda x: bytes.fromhex(x)})
        # create a new column with the WKB representation of the geometry
        gdf['geometry'] = gdf.geom.apply(lambda x: shapely.wkb.loads(x))
        # remove the old geometry column
        gdf.drop('geom', axis=1, inplace=True)
    else:
        gdf = gpd.read_csv(csv_file)
    return gdf

def load_arrow_to_geopandas(arrow_file, is_wkb=True):
    """
    Load an Arrow file into a GeoPandas dataframe.
    """
    gdf = gpd.read_feather(arrow_file, use_threads=True)
    print(gdf)

geojson_file_path = os.path.join(os.path.dirname(__file__), "../data/guerry.geojson")
csv_wkt_file_path = os.path.join(os.path.dirname(__file__), "../data/guerry_wkt.csv")
csv_wkb_file_path = os.path.join(os.path.dirname(__file__), "../data/guerry_wkb.csv")
arrow_file_path = os.path.join(os.path.dirname(__file__), "../data/guerry.arrow")

read_geojson_to_wkt(geojson_file_path)
save_geojson_to_csv(geojson_file_path, csv_wkt_file_path, is_wkb=False)
save_geojson_to_csv(geojson_file_path, csv_wkb_file_path, is_wkb=True)
save_geojson_to_arrow(geojson_file_path, arrow_file_path)
load_arrow_to_geopandas(arrow_file_path)
