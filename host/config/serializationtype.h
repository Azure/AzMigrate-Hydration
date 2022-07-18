#ifndef SERIALIZE__TYPE__H_
#define SERIALIZE__TYPE__H_

typedef enum ESERIALIZE_TYPE
{
    None = -1,
    PHPSerialize, /* TODO: should this be php serialize ? */
    Xmlize

} SerializeType_t;

#endif /* SERIALIZE__TYPE__H_ */
