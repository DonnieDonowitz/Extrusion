#include <osg/Geode>
#include <osg/Array>
#include <osg/Geometry>
#include <osgUtil/Tessellator>
#include <osgViewer/Viewer>

/*
    Function createUpperFace
    @param  vertices                :   Array di Vec3 contenenti le coordinate dei vertici esterni della figura;
    @param  holes                   :   Lista di Array di Vec3 contenenti le coordinate dei vertici dei fori;
    @param  color                   :   Array di Vec4 contenenti il/i colore/i del solido;
    @param  extrusiondirection      :   Vec3 contenente la direzione di estrusione.
*/

osg::Geometry* createUpperFace(osg::Vec3Array* vertices, std::list<osg::Vec3Array*> holes, osg::Vec4Array* color, osg::Vec3 extrusiondirection)
{
    int off;
    osg::Vec3Array* v = new osg::Vec3Array; //Array di Vec3 contenente tutti i vertici di fori e perimetro esterno;
    osg::Vec3Array* norms = new osg::Vec3Array; //Array di Vec3 contenente la normale alla superficie;
 
    v->insert(v->begin(), vertices->begin(), vertices->end());
    osg::Vec3 n = (v->at(1) - v->at(0)) ^ (v->at(2) - v->at(1));
    n.normalize();
    norms->push_back(n);
    off = vertices->size();
    for (osg::Vec3Array* h : holes)
    {
        v->insert(v->begin() + off, h->begin(), h->end());
        off += h->size();
    }

    osgUtil::Tessellator t;
    t.setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
    t.setTessellationType(osgUtil::Tessellator::TESS_TYPE_GEOMETRY);
 
    osg::Geometry* face = new osg::Geometry;
    face->setColorArray(color, osg::Array::BIND_OVERALL);
    face->setVertexArray(v);
    face->setNormalArray(norms, osg::Array::BIND_OVERALL);
    face->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, vertices->size()));
    off = vertices->size();
    for (osg::Vec3Array* h : holes)
    {
        face->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, off, h->size()));
        off += h->size();
    }
    t.retessellatePolygons(*face); //Tessellamento della superficie;
    return face;
}

/*
    Function createLowerFace
    @param  upperFace               :   Geometry contenente la faccia superiore del solido tessellata;
    @param  offset                  :   Vettore di estrusione;
*/

osg::Geometry* createLowerFace(osg::Geometry* upperFace, osg::Vec3 offset)
{
    osg::Geometry* lowerFace = dynamic_cast<osg::Geometry*> (upperFace->clone(osg::CopyOp::DEEP_COPY_ALL));
    osg::Vec3Array* v = (osg::Vec3Array*) lowerFace->getVertexArray();
    
    for (int i = 0; i < v->size(); ++i)
    {
        v->at(i) += offset;
    }

    osg::Vec3Array* norm = new osg::Vec3Array;
    osg::Vec3 n = ((v->at(1)) - (v->at(0))) ^ ((v->at(2)) - (v->at(1)));
    n.normalize();
    norm->push_back(- n);

    lowerFace->setNormalArray(norm, osg::Array::BIND_OVERALL);
    return lowerFace;
}

/*
    Function createWalls
    @param  vertices                :   Array di Vec3 contenenti le coordinate dei vertici esterni della figura;
    @param  holes                   :   Lista di Array di Vec3 contenenti le coordinate dei vertici dei fori;
    @param  color                   :   Array di Vec4 contenenti il/i colore/i del solido;
    @param  extrusiondirection      :   Vec3 contenente la direzione di estrusione;
    @param  magnitude               :   Magnitudo di estrusione.
*/

osg::Geometry* createWalls(osg::Vec3Array* vertices, std::list<osg::Vec3Array*> holes, osg::Vec4Array* color, osg::Vec3 extrusiondirection, double magnitude)
{
    int off;
    int numholes = holes.size();
    int numvertices = vertices->size();
    int numvertexholes = 0;

    osg::Vec3Array* v = new osg::Vec3Array; //Array di Vec3 contenente tutti i vertici di fori e perimetro esterno;
    osg::Vec3Array* norms = new osg::Vec3Array; //Array di Vec3 contenente le normali alle superfici;
    osg::Vec3 offset = extrusiondirection * magnitude;  //Offset di estrusione;
    osg::Geometry* walls = new osg::Geometry;

    v->insert(v->begin(), vertices->begin(), vertices->end());
    for (osg::Vec3Array::iterator itr = vertices->begin(); itr != vertices->end(); ++itr)
    {
        v->push_back((*itr) + offset);
    }

    off = 2 * numvertices;
    for (osg::Vec3Array* h : holes)
    {
        v->insert(v->begin() + off, h->begin(), h->end());
        for (osg::Vec3Array::iterator itr = h->begin(); itr != h->end(); ++itr)
        {
            numvertexholes++;
            v->push_back((*itr) + offset);
        }
        off += 2 * h->size();
    }

    walls->setColorArray(color, osg::Array::BIND_OVERALL);
    walls->setVertexArray(v);
    
    std::vector<osg::DrawElementsUInt *> indices(numvertices);
    for (int i = 0; i < numvertices; ++i)
    {
        indices[i] = new osg::DrawElementsUInt(GL_QUAD_STRIP);
        indices[i]->push_back(i);
        indices[i]->push_back(i + numvertices);
        indices[i]->push_back((i + 1) % numvertices);
        
        int last = (i + numvertices + 1) % (2 * numvertices);
        if (last == 0)
        {
            indices[i]->push_back(last + numvertices);
        }
        else
        {
            indices[i]->push_back(last);
        }
        osg::Vec3 n = (v->at(indices[i]->at(1)) - v->at(indices[i]->at(0))) ^ (v->at(indices[i]->at(2)) - v->at(indices[i]->at(1)));;
        n.normalize();
        norms->push_back(n);
        walls->addPrimitiveSet(indices[i]);
    }

    std::vector<osg::DrawElementsUInt *> indicesholes(numvertexholes);
    off = 2 * numvertices;
    int j = 0;
    for (osg::Vec3Array* h : holes)
    {
        int s = h->size();
        for (int i = off; i < off + s; ++i)
        {
            indicesholes[j] = new osg::DrawElementsUInt(GL_QUAD_STRIP);
            indicesholes[j]->push_back(i);
            indicesholes[j]->push_back(i + s);

            int l = (i + 1) % (off + s);
            if (l == 0)
            {
                indicesholes[j]->push_back(l + off);
            }
            else
            {
                indicesholes[j]->push_back(l);
            }
            
            l = (i + s + 1) % (off + 2 * s);
            if (l == 0)
            {
                indicesholes[j]->push_back(l + off + s);
            }
            else
            {
                indicesholes[j]->push_back(l);
            }
            osg::Vec3 n = (v->at(indicesholes[j]->at(1)) - v->at(indicesholes[j]->at(0))) ^ (v->at(indicesholes[j]->at(2)) - v->at(indicesholes[j]->at(1)));
            n.normalize();
            norms->push_back(n);
            walls->addPrimitiveSet(indicesholes[j]);
            j++;
        }
        off += 2 * s;
    }
    walls->setNormalArray(norms, osg::Array::BIND_PER_PRIMITIVE_SET);
    return walls;
}

/*
    Function createExtrusion
    @param  vertices                :   Array di Vec3 contenenti le coordinate dei vertici esterni della figura;
    @param  holes                   :   Lista di Array di Vec3 contenenti le coordinate dei vertici dei fori;
    @param  color                   :   Array di Vec4 contenenti il/i colore/i del solido;
    @param  extrusiondirection      :   Vec3 contenente la direzione di estrusione;
    @param  magnitude               :   Magnitudo di estrusione.
*/

osg::Geode* createExtrusion(osg::Vec3Array* extborder, std::list<osg::Vec3Array*> holesborders, osg::Vec4Array* color, osg::Vec3 extrusiondirection, double magnitude)
{
    osg::Geode* geo = new osg::Geode;

    osg::Vec3 offset = extrusiondirection * magnitude;

    osg::Geometry* upperFace = createUpperFace(extborder, holesborders, color, extrusiondirection);
    osg::Geometry* lowerFace = createLowerFace(upperFace, offset);

    geo->addDrawable(upperFace);
    geo->addDrawable(lowerFace);
    geo->addDrawable(createWalls(extborder, holesborders, color, extrusiondirection, magnitude));
    return geo;
}

//Esempio di utilizzo
int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Group> root = new osg::Group;
    std::list<osg::Vec3Array*> listHole;
    
    osg::Vec3Array* vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(-9.0f, 0.0, 5.0f));
    vertices->push_back(osg::Vec3(-9.0f, 0.0, -4.0f));
    vertices->push_back(osg::Vec3(-1.0f, 0.0, -4.0f));
    vertices->push_back(osg::Vec3(-1.0f, 0.0, -2.0f));
    vertices->push_back(osg::Vec3(-7.0f, 0.0, -2.0f));
    vertices->push_back(osg::Vec3(-7.0f, 0.0, 5.0f));

    osg::Vec3Array* hole = new osg::Vec3Array;
    hole->push_back(osg::Vec3(-8.22f, 0.0, -2.87f));
    hole->push_back(osg::Vec3(-6.94f, 0.0, -2.87f));
    hole->push_back(osg::Vec3(-6.96f, 0.0, -3.53f));
    hole->push_back(osg::Vec3(-8.48f, 0.0, -3.53f));
    hole->push_back(osg::Vec3(-8.48f, 0.0, -2.01f));

    osg::Vec3Array* hole2 = new osg::Vec3Array;
    hole2->push_back(osg::Vec3(-3.94f, 0.0, -2.27f));
    hole2->push_back(osg::Vec3(-3.8f, 0.0, -3.39f));
    hole2->push_back(osg::Vec3(-2.9f, 0.0, -2.37f));
    hole2->push_back(osg::Vec3(-1.42f, 0.0, -3.05f));
    hole2->push_back(osg::Vec3(-3.46f, 0.0, -3.81f));
    hole2->push_back(osg::Vec3(-6.2f, 0.0, -3.29f));
    hole2->push_back(osg::Vec3(-5.54f, 0.0, -2.39f));

    osg::Vec3Array* hole3 = new osg::Vec3Array;
    hole3->push_back(osg::Vec3(-8.26f, 0.0, 3.43f)); 
    hole3->push_back(osg::Vec3(-8.5f, 0.0, 4.63f));
    hole3->push_back(osg::Vec3(-7.48f, 0.0, 4.55f));
   
    osg::Vec3Array* hole4 = new osg::Vec3Array;
    hole4->push_back(osg::Vec3(-7.36f, 0.0, 3.05f));
    hole4->push_back(osg::Vec3(-8.2f, 0.0, 1.45f));
    hole4->push_back(osg::Vec3(-8.52f, 0.0, 2.45f));
    hole4->push_back(osg::Vec3(-7.76f, 0.0, 3.39f));

    osg::Vec3Array* hole5 = new osg::Vec3Array;
    hole5->push_back(osg::Vec3(-7.36f, 0.0, 0.47f));
    hole5->push_back(osg::Vec3(-8.58f, 0.0, 0.43f));
    hole5->push_back(osg::Vec3(-7.48f, 0.0, 1.77f));

    osg::Vec3Array* hole6 = new osg::Vec3Array;
    hole6->push_back(osg::Vec3(-7.32f, 0.0, -1.35f)); 
    hole6->push_back(osg::Vec3(-8.44f, 0.0, -0.17f));
    hole6->push_back(osg::Vec3(-7.34f, 0.0, -0.17f));
    
    osg::Vec3Array* hole7 = new osg::Vec3Array;
    hole7->push_back(osg::Vec3(-8.0f, 0.0, -1.37f));
    hole7->push_back(osg::Vec3(-7.62f, 0.0, -1.55f));
    hole7->push_back(osg::Vec3(-7.74f, 0.0, -2.33f));
    hole7->push_back(osg::Vec3(-8.44f, 0.0, -1.37f));
    hole7->push_back(osg::Vec3(-8.42f, 0.0, -0.83f));

    listHole.push_back(hole);
    listHole.push_back(hole2);
    listHole.push_back(hole3);
    listHole.push_back(hole4);
    listHole.push_back(hole5);
    listHole.push_back(hole6);
    listHole.push_back(hole7);

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
    (*colors)[0].set(0.0856f, 0.5273f, 0.6523f, 1.0f);

    osg::ref_ptr<osg::Geode> geode = createExtrusion(vertices, listHole, colors, osg::Vec3(0.0f, 1.0f, 0.0f), 15.0f);

    root->addChild(geode.get());

    osgViewer::Viewer viewer;
    viewer.setSceneData(root.get());
    return viewer.run();
}
