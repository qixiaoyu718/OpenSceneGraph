#include <osgUtil/RenderBin>
#include <osgUtil/RenderStage>

using namespace osg;
using namespace osgUtil;


// register a RenderStage prototype with the RenderBin prototype list.
RegisterRenderBinProxy<RenderBin> s_registerRenderBinProxy;

typedef std::map< std::string, osg::ref_ptr<RenderBin> > RenderBinPrototypeList;

RenderBinPrototypeList* renderBinPrototypeList()
{
    static RenderBinPrototypeList s_renderBinPrototypeList;
    return &s_renderBinPrototypeList;
}

RenderBin* RenderBin::createRenderBin(const std::string& binName)
{
//    cout << "creating RB "<<binName<<endl;

    RenderBinPrototypeList::iterator itr = renderBinPrototypeList()->find(binName);
    if (itr != renderBinPrototypeList()->end()) return dynamic_cast<RenderBin*>(itr->second->clone());
    else return NULL;
}

void RenderBin::addRenderBinPrototype(RenderBin* proto)
{
    if (proto)
    {
        (*renderBinPrototypeList())[proto->className()] = proto;
//        cout << "Adding RB "<<proto->className()<<endl;
    }
}

void RenderBin::removeRenderBinPrototype(RenderBin* proto)
{
    if (proto)
    {
        RenderBinPrototypeList::iterator itr = renderBinPrototypeList()->find(proto->className());
        if (itr != renderBinPrototypeList()->end()) renderBinPrototypeList()->erase(itr);
    }
}

RenderBin::RenderBin()
{
    _binNum = 0;
    _parent = NULL;
    _stage = NULL;
}

RenderBin::~RenderBin()
{
}

void RenderBin::reset()
{
    _renderGraphList.clear();
    _bins.clear();
}

void RenderBin::sort()
{
    for(RenderBinList::iterator itr = _bins.begin();
        itr!=_bins.end();
        ++itr)
    {
        itr->second->sort();
    }
    sort_local();
}

RenderBin* RenderBin::find_or_insert(int binNum,const std::string& binName)
{
    // search for appropriate bin.
    RenderBinList::iterator itr = _bins.find(binNum);
    if (itr!=_bins.end()) return itr->second.get();

    // create a renderin bin and insert into bin list.
    RenderBin* rb = RenderBin::createRenderBin(binName);
    if (rb)
    {

        RenderStage* rs = dynamic_cast<RenderStage*>(rb);
        if (rs)
        {
            rs->_binNum = binNum;
            rs->_parent = NULL;
            rs->_stage = rs;
            _stage->addToDependencyList(rs);
        }
        else
        {
            rb->_binNum = binNum;
            rb->_parent = this;
            rb->_stage = _stage;
            _bins[binNum] = rb;
        }
    }
    return rb;
}

void RenderBin::draw(osg::State& state,RenderLeaf*& previous)
{
    // draw first set of draw bins.
    RenderBinList::iterator itr;
    for(itr = _bins.begin();
        itr->first<0 && itr!=_bins.end();
        ++itr)
    {
        itr->second->draw(state,previous);
    }
    
    draw_local(state,previous);

    for(;
        itr!=_bins.end();
        ++itr)
    {
        itr->second->draw(state,previous);
    }
}

void RenderBin::draw_local(osg::State& state,RenderLeaf*& previous)
{
    // draw local bin.
    for(RenderGraphList::iterator oitr=_renderGraphList.begin();
        oitr!=_renderGraphList.end();
        ++oitr)
    {

        for(RenderGraph::LeafList::iterator dw_itr = (*oitr)->_leaves.begin();
            dw_itr != (*oitr)->_leaves.end();
            ++dw_itr)
        {
            RenderLeaf* rl = dw_itr->get();
            rl->render(state,previous);
            previous = rl;

        }
    }
}
// stats
#include <osg/GeoSet>
void RenderBin::getPrims(Statistics *primStats)
{
    for(RenderBinList::iterator itr = _bins.begin();
    itr!=_bins.end();
    ++itr)
    {
                primStats->nbins++;
    }
    
    for(RenderGraphList::iterator oitr=_renderGraphList.begin();
    oitr!=_renderGraphList.end();
    ++oitr)
    {
        
        for(RenderGraph::LeafList::iterator dw_itr = (*oitr)->_leaves.begin();
            dw_itr != (*oitr)->_leaves.end();
            ++dw_itr)
        {
            RenderLeaf* rl = dw_itr->get();
            Drawable* dw= rl->_drawable;
            primStats->numOpaque++; // number of geosets
            if (rl->_matrix.get()) primStats->nummat++; // number of matrices
            if (dw) { // then tot up the types 1-14
                GeoSet *gs=dynamic_cast<osg::GeoSet*>(dw);
                if (gs)
                {
                    primStats->addstat(gs);
                    //        rl->getPrims(state,previous);??
                    //        previous = rl;
                }
            }
        }
    }
}
