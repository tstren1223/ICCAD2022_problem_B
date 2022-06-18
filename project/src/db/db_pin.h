#ifndef _DB_PIN_H_
#define _DB_PIN_H_

#include "../sta/sta.h"

namespace db {
class PinType {
private:
    string _name = "";
    //  i: input, o:output
    char _direction = 'x';
    // s: signal, c: clk, p: power, g: ground
    char _type = 's';

public:
    vector<Geometry> shapes;
    int boundLX = INT_MAX;
    int boundLY = INT_MAX;
    int boundHX = INT_MIN;
    int boundHY = INT_MIN;

    PinType(const string& name, const char direction, const char type) : _name(name), _direction(direction), _type(type) {}

    const string& name() const { return _name; }
    char direction() const { return _direction; }
    char type() const { return _type; }

    void direction(const char c) { _direction = c; }

    void addShape(const Layer& layer, const int lx, const int ly, const int hx, const int hy);
    void addShape(const Layer& layer, const int lx, const int ly) { addShape(layer, lx, ly, lx + 1, ly + 1); }
    unsigned getW() const { return boundHX - boundLX; }
    unsigned getH() const { return boundHY - boundLY; }
    void getBounds(int& lx, int& ly, int& hx, int& hy) const;
    bool operator==(const PinType& r) const {
        return _type == r._type && boundLX == r.boundLX && boundLY == r.boundLY && boundHX == r.boundHX &&
               boundHY == r.boundHY && shapes[0].layer.rIndex == r.shapes[0].layer.rIndex;
    }
    bool operator!=(const PinType& r) const { return !(*this == r); }
    bool operator<(const PinType& r) const;
    bool operator>(const PinType& r) const { return r < *this; }
    bool operator>=(const PinType& r) const { return !(*this < r); }
    bool operator<=(const PinType& r) const { return !(*this > r); }
    static bool comparePin(vector<PinType*> pins1, vector<PinType*> pins2);
};

class IOPin : virtual public Drawable {
protected:
    string _netName = "";

public:
    string name = "";
    int x = INT_MIN;
    int y = INT_MIN;
    PinType* type;
    Pin* pin;

    IOPin(const string& name = "", const string& netName = "", const char direction = 'x');
    ~IOPin();

    const string& netName() const { return _netName; }
    const int width() const { return this->type->getW(); }
    const int height() const { return this->type->getH(); }
    const int lx() const { return x; }
    const int ly() const { return y; }
    const int hx() const { return x + width(); }
    const int hy() const { return y + height(); }
    const int cx() const { return x + width() / 2; }
    const int cy() const { return y + height() / 2; }
    void getBounds(int& lx, int& ly, int& hx, int& hy, int& rIndex) const;

    /*Drawable*/
    int globalX() const;
    int globalY() const;
    int localX() const;
    int localY() const;
    int boundL() const;
    int boundR() const;
    int boundB() const;
    int boundT() const;

    /*GDDrawable*/
    Drawable* parent() const;
    void draw(Visualizer* v) const;
    void draw(Visualizer* v, Layer& L) const;
};

class PinSTA {
public:
    char type;  //'b': timing begin, 'e': timing end, 'i': intermediate
    double capacitance;
    double aat[4];
    double rat[4];
    double slack[4];
    int nCriticalPaths[4];
    PinSTA();
};

class Pin : virtual public Drawable {
public:
    Cell* cell = nullptr;
    IOPin* iopin = nullptr;
    Net* net = nullptr;
    const PinType* type = nullptr;

    PinSTA* staInfo = nullptr;

    Pin(const PinType* type = nullptr) : type(type) {}
    Pin(Cell* cell, int i) : cell(cell), type(cell->ctype()->pins[i]) {}
    Pin(IOPin* iopin) : iopin(iopin), type(iopin->type) {}
    Pin(const Pin& pin) : cell(pin.cell), net(pin.net), type(pin.type) {}

    void getPinCenter(int& x, int& y);
    void getPinBounds(int& lx, int& ly, int& hx, int& hy, int& rIndex);

    utils::BoxT<double> getPinParentBBox() const;
    utils::PointT<int> getPinParentCenter() const;

    /*Drawable*/
    int globalX() const;
    int globalY() const;
    int localX() const;
    int localY() const;
    int boundL() const;
    int boundR() const;
    int boundB() const;
    int boundT() const;

    Drawable* parent() const;
    void draw(Visualizer* v) const;
    void draw(Visualizer* v, const Layer& L) const;
};  // namespace db
}  // namespace db

#endif
