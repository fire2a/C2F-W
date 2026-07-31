#ifndef READSHP_H
#define READSHP_H
// ---------------------------------------------------------------------------
// Lector minimo de shapefile ESRI (.shp) para PolyLine (tipo 3) y Polygon
// (tipo 5), sin dependencias externas (no GDAL/OGR). Devuelve las "partes"
// (polilineas) como vectores de puntos (x,y) en la CRS del archivo.
// Se asume que el .shp ya viene reproyectado a la CRS de la instancia (UTM).
// ---------------------------------------------------------------------------
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

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

#endif  // READSHP_H
