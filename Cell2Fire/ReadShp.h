#ifndef READSHP_H
#define READSHP_H
// ---------------------------------------------------------------------------
// Lector minimo de shapefile ESRI (.shp) para PolyLine (tipo 3) y Polygon
// (tipo 5), sin dependencias externas (no GDAL/OGR). Devuelve las "partes"
// (polilineas) como vectores de puntos (x,y) en la CRS del archivo.
// Se asume que el .shp ya viene reproyectado a la CRS de la instancia (UTM).
// ---------------------------------------------------------------------------
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// MSVC no define S_ISREG (solo _S_IFMT/_S_IFREG)
#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif

struct ShpPoint
{
    double x, y;
};
typedef std::vector<ShpPoint> ShpPart;  // una polilinea (o anillo de poligono)

// lee un double little-endian
static inline double
readLE_double(const unsigned char* p)
{
    double v;
    std::memcpy(&v, p, 8);  // x86/ARM son little-endian
    return v;
}
static inline int32_t
readLE_int(const unsigned char* p)
{
    int32_t v;
    std::memcpy(&v, p, 4);
    return v;
}
static inline int32_t
readBE_int(const unsigned char* p)
{
    return (int32_t(p[0]) << 24) | (int32_t(p[1]) << 16) | (int32_t(p[2]) << 8) | int32_t(p[3]);
}

// Lee todas las partes (polilineas/anillos) del shapefile.
// Retorna false si no se pudo abrir o el tipo no es PolyLine/Polygon.
inline bool
readShapefileParts(const std::string& path, std::vector<ShpPart>& outParts)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) return false;
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (buf.size() < 100) return false;

    int32_t fileCode = readBE_int(&buf[0]);
    if (fileCode != 9994) return false;                 // no es shapefile
    int32_t shpType = readLE_int(&buf[32]);
    if (shpType != 3 && shpType != 5) return false;     // solo PolyLine/Polygon

    size_t pos = 100;                                   // fin de la cabecera
    while (pos + 8 <= buf.size())
    {
        // record header (big-endian): numero (4) + longitud de contenido en words (4)
        int32_t contentLenWords = readBE_int(&buf[pos + 4]);
        size_t recStart = pos + 8;
        size_t recLen = size_t(contentLenWords) * 2;    // words -> bytes
        if (recStart + recLen > buf.size()) break;

        int32_t recType = readLE_int(&buf[recStart]);
        if (recType == 3 || recType == 5)               // PolyLine / Polygon
        {
            // layout: type(4) box(32) numParts(4) numPoints(4) parts[np] points[npt*16]
            size_t p = recStart + 4 + 32;
            int32_t numParts = readLE_int(&buf[p]);       p += 4;
            int32_t numPoints = readLE_int(&buf[p]);      p += 4;
            std::vector<int32_t> parts(numParts);
            for (int i = 0; i < numParts; ++i) { parts[i] = readLE_int(&buf[p]); p += 4; }
            size_t ptBase = p;                            // inicio del array de puntos
            for (int ip = 0; ip < numParts; ++ip)
            {
                int start = parts[ip];
                int end = (ip + 1 < numParts) ? parts[ip + 1] : numPoints;
                ShpPart part;
                for (int k = start; k < end; ++k)
                {
                    const unsigned char* pp = &buf[ptBase + size_t(k) * 16];
                    ShpPoint sp{ readLE_double(pp), readLE_double(pp + 8) };
                    part.push_back(sp);
                }
                if (part.size() >= 2) outParts.push_back(part);
            }
        }
        pos = recStart + recLen;
    }
    return !outParts.empty();
}

// Lee los puntos de un shapefile de tipo Point (1) o MultiPoint (8), incluyendo
// las variantes Z/M que exporta QGIS (11/21 y 18/28). Los puntos salen en el
// orden del archivo, que es el que define a que anio corresponde cada ignicion.
// Retorna false si no se pudo abrir, no es shapefile, o el tipo no es de puntos.
inline bool
readShapefilePoints(const std::string& path, std::vector<ShpPoint>& outPoints)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) return false;
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (buf.size() < 100) return false;

    if (readBE_int(&buf[0]) != 9994) return false;      // no es shapefile
    int32_t shpType = readLE_int(&buf[32]);
    const bool isPoint = (shpType == 1 || shpType == 11 || shpType == 21);
    const bool isMulti = (shpType == 8 || shpType == 18 || shpType == 28);
    if (!isPoint && !isMulti) return false;

    size_t pos = 100;
    while (pos + 8 <= buf.size())
    {
        int32_t contentLenWords = readBE_int(&buf[pos + 4]);
        size_t recStart = pos + 8;
        size_t recLen = size_t(contentLenWords) * 2;
        if (recStart + recLen > buf.size()) break;

        int32_t recType = readLE_int(&buf[recStart]);
        if (recType == 1 || recType == 11 || recType == 21)
        {
            // layout: type(4) x(8) y(8) [z(8)] [m(8)]
            const unsigned char* pp = &buf[recStart + 4];
            outPoints.push_back(ShpPoint{ readLE_double(pp), readLE_double(pp + 8) });
        }
        else if (recType == 8 || recType == 18 || recType == 28)
        {
            // layout: type(4) box(32) numPoints(4) points[n*16] [z...] [m...]
            size_t p = recStart + 4 + 32;
            int32_t numPoints = readLE_int(&buf[p]);
            p += 4;
            for (int k = 0; k < numPoints; ++k)
            {
                const unsigned char* pp = &buf[p + size_t(k) * 16];
                outPoints.push_back(ShpPoint{ readLE_double(pp), readLE_double(pp + 8) });
            }
        }
        pos = recStart + recLen;
    }
    return !outPoints.empty();
}

// Lee el bounding box del header del .shp (bytes 36..67: Xmin, Ymin, Xmax, Ymax).
// Sirve para avisar temprano si la capa no solapa el raster, que casi siempre
// significa CRS equivocada. No requiere parsear los registros.
inline bool
readShapefileBBox(const std::string& path, double& xmin, double& ymin, double& xmax, double& ymax)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) return false;
    unsigned char h[100];
    f.read(reinterpret_cast<char*>(h), 100);
    if (f.gcount() < 100) return false;
    if (readBE_int(&h[0]) != 9994) return false;
    xmin = readLE_double(&h[36]);
    ymin = readLE_double(&h[44]);
    xmax = readLE_double(&h[52]);
    ymax = readLE_double(&h[60]);
    return true;
}

// Lista los .shp de una carpeta devolviendo {ruta, tipo}, donde el tipo es el nombre
// del archivo sin extension (Barriers/roads.shp -> "roads"). Orden alfabetico para
// que el resultado sea reproducible entre corridas y sistemas de archivos.
inline std::vector<std::pair<std::string, std::string>>
listShapefiles(const std::string& dir)
{
    std::vector<std::pair<std::string, std::string>> out;
    DIR* d = opendir(dir.c_str());
    if (!d) return out;  // la carpeta es opcional
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL)
    {
        const std::string name = ent->d_name;
        if (name.size() < 5) continue;
        if (name.compare(name.size() - 4, 4, ".shp") != 0) continue;
        const std::string full = dir + "/" + name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;
        out.push_back({ full, name.substr(0, name.size() - 4) });
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}

// Tipo de geometria del shapefile (1=Point, 3=PolyLine, 5=Polygon, 8=MultiPoint,
// y sus variantes Z/M). Devuelve -1 si no se pudo leer.
inline int
readShapefileType(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) return -1;
    unsigned char h[100];
    f.read(reinterpret_cast<char*>(h), 100);
    if (f.gcount() < 100) return -1;
    if (readBE_int(&h[0]) != 9994) return -1;
    return readLE_int(&h[32]);
}

// Punto dentro de poligono (ray casting sobre todos los anillos, regla par-impar,
// de modo que los anillos interiores se comportan como huecos). Los anillos vienen
// de readShapefileParts, que para Polygon devuelve cada anillo como una "parte".
inline bool
pointInPolygon(const std::vector<ShpPart>& rings, double x, double y)
{
    bool inside = false;
    for (const auto& ring : rings)
    {
        const size_t n = ring.size();
        if (n < 3) continue;
        for (size_t i = 0, j = n - 1; i < n; j = i++)
        {
            const double xi = ring[i].x, yi = ring[i].y;
            const double xj = ring[j].x, yj = ring[j].y;
            if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi + 1e-300) + xi))
                inside = !inside;
        }
    }
    return inside;
}

#endif  // READSHP_H
