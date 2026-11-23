#ifndef GET_PARAM_H
#define GET_PARAM_H

#define GET_PARAM(type, node) \
    *(type*)node->params; \
    node->params += sizeof(type);

#endif