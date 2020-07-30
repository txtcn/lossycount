#include "lossycount.h"
#include <boost/python.hpp>
#include <boost/python/list.hpp>
#include <boost/python/tuple.hpp>
#include <iostream>
#include <string>
#include <vector>
/*
extern LCL_type * LCL_Init(float fPhi);
extern void LCL_Destroy(LCL_type *);
extern void LCL_Update(LCL_type *, LCLitem_t, int);
extern unsigned LCL_Size(LCL_type *);
extern int LCL_PointEst(LCL_type *, LCLitem_t);
extern int LCL_PointErr(LCL_type *, LCLitem_t);
extern std::map<uint32_t, uint32_t> LCL_Output(LCL_type *,int);
*/
namespace { 
using namespace boost::python;
/*
vector<string> word_vec(char* c,unsigned win_width){
    vector<string> str;
    unsigned pos=0;
    unsigned pre_pos=0;
    unsigned window_length=0;
    while(c[pos++]){
        str.push_back(string(pre_pos,pos));  
    }
    return str;
} 
*/
class LossyCount{
    LCL_type* _lcl;
    public:
        LossyCount(float phi):
            _lcl(LCL_Init(phi))
        {
        }
        
        void destroy(){
            LCL_Destroy(_lcl);
        }
        
        void incr(LCLitem_t item,int value=1){
            LCL_Update(_lcl,item,value);
        }

        unsigned capacity(){
            return LCL_Size(_lcl); 
        }
        
        LCLweight_t est(LCLitem_t k){
            return LCL_PointEst(_lcl,k);
        }        
        LCLweight_t err(LCLitem_t k){
            return LCL_PointErr(_lcl,k);
        } 

        list output(LCLweight_t thresh){
            list res;

            for (int i=1;i<=_lcl->size;++i)
            {
                LCLCounter& counters = _lcl->counters[i];
                if (counters.count>=thresh)
                    res.append(
                        make_tuple(counters.item, counters.count)
                    );
            }

            return res;
        }

};

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(incr_overloads, incr, 1, 2);

BOOST_PYTHON_MODULE(lossycount)
{
    namespace python = boost::python;    

    class_<LossyCount>("LossyCount",init<float>())
        .def("incr",&LossyCount::incr, incr_overloads())
        .def("err",&LossyCount::err)
        .def("output",&LossyCount::output)
        .def("est",&LossyCount::est)
        .def("__del__",&LossyCount::destroy)
        .def("capacity",&LossyCount::capacity);


}
}
