#include "parser.h"

bool is_SUL_parsed = false;

void init_SUL(SUL_t *sul){
    sul->sequence_number = 1;
    memset(&sul->version, '\0', 6);
    memset(&sul->structure, '\0', 7);
    memset(&sul->max_length, '\0', 6);
    memset(&sul->id, '\0', 61);
}

void init_parser(parser_t *parser) {
    init_SUL(&parser->sul);
	cbuffer_init(&parser->cbuffer);
    init_visible_record(&parser->root_vr);
    parser->current_vr = &parser->root_vr;
    init_logical_record(&parser->root_lr);
    parser->current_lr = &parser->root_lr;
    //init_lr_segment(&parser->current_segment);
}

void init_visible_record(visible_record_t *vr){
    print_log("");
    vr->length = 0;
    vr->version = 1;
    vr->c_idx = 0;
    vr->next = NULL;
}

void init_logical_record(logical_record_t *lr){
    lr->type = NULL;
    lr->name = NULL;
    lr->next = NULL;
    init_lr_attribute(&lr->template);
    lr->current_attribute = &lr->template;

    // init_lr_object(&lr->objects);
    lr->current_obj = &lr->objects;
    lr->is_template_completed = false;
}

void init_lr_segment(lr_segment_t *lr_segment){
    lr_segment->length = 0;
    // lr_segment->attributes = '\0';
    lr_segment->type = 0;
    lr_segment->structure = true;
    lr_segment->predecessor = false;
    lr_segment->successor = true;
    lr_segment->encryption = false;
    lr_segment->encryption_packet = false;
    lr_segment->checksum = false;
    lr_segment->trailing_length = false;
    lr_segment->padding = false;
}

void init_lr_attribute(lr_attribute_t* attr){
    attr->label = NULL;
    attr->count = 1;
    attr->representation_code = DLIS_IDENT;
    attr->unit = NULL;
    attr->value = NULL;
    attr->next = NULL;
}

void init_lr_object(lr_object_t* obj, lr_attribute_t* attr){
    printf("init_lr_object >>>>>%d ", attr->representation_code);
    obj->name = NULL;
    obj->origin = true;
    obj->copy_number = 0;
    init_lr_attribute(&obj->attributes);
    obj->current_attr = &obj->attributes;
    obj->next = NULL;

    //copy attributes
    obj->attributes.representation_code = attr->representation_code;
    
    while(attr->next != NULL){
        attr = attr->next;

        lr_attribute_t* tmp = malloc(sizeof(lr_attribute_t));
        init_lr_attribute(tmp);
        tmp->representation_code = attr->representation_code;

        obj->current_attr->next = tmp;
        obj->current_attr = obj->current_attr->next;
        printf(" >> %d ", obj->current_attr->representation_code);
    }
    printf("\n");
    obj->current_attr = &obj->attributes;
}

void init_obname_t(obname_t* obname){
    obname->name = NULL;
    obname->origin = 0;
    obname->copy_number = 0;
}


void parser_parses(parser_t *parser, char * file_path){
    FILE *file = fopen(file_path, "rb");
    unsigned char buffer[BUFF_SIZE];
    memset(&buffer, '\0', BUFF_SIZE);    
    long long size = ftell(file);
    printf("===>> %lld", size);
    rewind(file);

    int bytes_a_time = 10000;
   
    while(!feof(file)){
        fread(buffer, 1, bytes_a_time, file);
        bool put_status = true;
        if(!feof(file)){
            put_status = put_data(parser, buffer, bytes_a_time);
        } else {
            printf("@@@@@@@@@@@@@@@> bytes %lld\n", size % bytes_a_time);
            put_status = put_data(parser, buffer, size % bytes_a_time);
        }     
        if(!put_status) break;
    }
    print_logical_record(parser);
    // print_visible_records(parser);
}

bool put_data(parser_t *parser,unsigned char *data, int length) {
    print_log("");
    bool insert_status = cbuffer_insert(&parser->cbuffer, data, length);
    if(!insert_status) return insert_status;
    if(!is_SUL_parsed){
        parse_sul(parser);
        is_SUL_parsed = true;
    }
    bool parsing_status = parse_visible_record(parser);
    //if(insert_status == true){
    //    parse_visible_record(parser);
    //}
    return parsing_status;
}



bool parse_sul(parser_t * parser){
    print_log("");
    unsigned char temp[6];
//    char* endptr;

    cbuffer_get(&parser->cbuffer, temp, 6, 4);
//    parser->sul.sequence_number = strtoimax((char *)temp, &endptr, 10);
    str_to_number(&parser->sul.sequence_number, temp, 4);
    cbuffer_get(&parser->cbuffer, parser->sul.version, 6, 5);
    cbuffer_get(&parser->cbuffer, parser->sul.structure, 7, 6);
    cbuffer_get(&parser->cbuffer, temp, 6, 5);
//    parser->sul.max_length = strtoull((char*) temp, &endptr, 10);
    str_to_number(&parser->sul.max_length, temp, 5);
    cbuffer_get(&parser->cbuffer, parser->sul.id, 61, 60);

    printf("%d\n", parser->sul.sequence_number);
    printf("%s\n", parser->sul.version);
    printf("%s\n", parser->sul.structure);
    printf("%llu\n", parser->sul.max_length);
    printf("%s\n", parser->sul.id);
    return true;
}



bool parse_visible_record(parser_t *parser){
    print_log("");
    printf("----------------------------->c_idx %d nbytes %d length %d\n", parser->current_vr->c_idx,parser->cbuffer.nbytes, parser->current_vr->length);
    if(parser->current_vr->c_idx == 0 && parser->cbuffer.nbytes > 4){
        //read visible record header
        unsigned char temp[5];
        cbuffer_get(&parser->cbuffer, temp, 5, 4);
        hex_to_number(&parser->current_vr->length,temp, 2);
        hex_to_number(&parser->current_vr->version, &temp[3],1);
        printf("visible envelop: %x", temp[2]);
        printf("vr length: %d, vr version %d\n", parser->current_vr->length, parser->current_vr->version);
        parser->current_vr->c_idx += 4;
    }
    if(parser->current_vr->c_idx == 0) return true;

    unsigned char tmp[parser->current_vr->length - 4]; // tru 4 vi co 4 byte header
    //read until finished current visible record
    int buffer_size = parser->cbuffer.nbytes;
    int vr_bytes_remain = parser->current_vr->length - parser->current_vr->c_idx;
    if(buffer_size < vr_bytes_remain){
        // cbuffer_get(&parser->cbuffer, &tmp[parser->current_vr->c_idx - 4], buffer_size, buffer_size);
        // parser->current_vr->c_idx += buffer_size;
        return true;
    } else {
        cbuffer_get(&parser->cbuffer, &tmp[parser->current_vr->c_idx - 4], vr_bytes_remain, vr_bytes_remain);
        parser->current_vr->c_idx += vr_bytes_remain;
        //end of visible record

        //parses logical record
        //tmp is a visible record
        parse_logical_record(parser, tmp, parser->current_vr->length - 4);
        

        parser->current_vr->next = malloc(sizeof(visible_record_t));
        init_visible_record(parser->current_vr->next);
        parser->current_vr = parser->current_vr->next;
        parse_visible_record(parser);
        return true;
    }
}

logical_record_t *getLR(parser_t *parser) {
	return NULL;
}


bool str_to_number(void *dest, void * data, int len){
    char * endptr;
    unsigned char temp[len+1];
    memset(temp, '\0', len+1);
    memmove(temp, data, len);
    if(len <= 4) {
        *(int*)dest = strtoimax((char*)temp, &endptr, 10);
        //*(int*)dest = atoi((char*)temp);
        return true;
    } else {
        *(long long*)dest = strtoll((char*)temp, &endptr, 10);
        return true;
    }
}
void print_visible_records(parser_t *parser){
    visible_record_t* next;
    next = &parser->root_vr;
    while(next != NULL){
        printf("==> visible record length: %d version %d\n", next->length, next->version);
        next = next->next;
    }

}

void parse_logical_record(parser_t* parser, unsigned char* body, int length){
    //current logical record parser->current_lr
    while (length > 0){
        //parse logical record segment
        //init segment
        printf("------>length %d\n", length);
        lr_segment_t current_segment;
        init_lr_segment(&current_segment);

        //read segment header
        hex_to_number(&current_segment.length, body, 2);
        body += 2;
        
        lr_segment_read_attributes(&current_segment, body[0]);
        body++;

                printf("1____lr_segment_read_attributes %d%d%d%d%d%d%d%d\n", current_segment.structure, current_segment.predecessor, 
            current_segment.successor, current_segment.encryption, current_segment.encryption_packet, 
            current_segment.checksum, current_segment.trailing_length, current_segment.padding);
        
        hex_to_number(&current_segment.type, body, 1);
        body++;
        

        // length -= current_segment.length - 4;

        //read segment body
        int segment_len_remain = current_segment.length - 4;
        if(current_segment.padding) segment_len_remain--;
        if(current_segment.checksum) segment_len_remain -= 2;
        if(current_segment.trailing_length) segment_len_remain -= 2;
        
        while(segment_len_remain > 0){
            //read every component
            int component_role = lr_read_component_role(body[0]);
            int component_format = lr_read_component_format(body[0]);
            body++;
            segment_len_remain--;
            
            switch (component_role){
                case ABSATR: 
                case ATTRIB:
                case INVATR: {
                    int nbytes = lr_read_attribute(parser->current_lr, component_format, body);
                    body += nbytes;
                    segment_len_remain -= nbytes;
                }
                    break;
                case OBJECT: {
                    parser->current_lr->is_template_completed = true;
                    int nbytes = lr_read_object(parser->current_lr, body);
                    body += nbytes;
                    segment_len_remain -= nbytes;
                }
                    break;
                case RDSET:
                case RSET:
                case SET: 
                    {
                        int len = 0;
                        if(component_format >= 10000){
                            hex_to_number(&len, body, 1);
                            body++;
                            segment_len_remain--;

                            parser->current_lr->type = malloc(len + 1); 
                            memmove(parser->current_lr->type, body, len);
                            parser->current_lr->type[len] = '\0';
                            body += len;
                            segment_len_remain -= len;
                            component_format -= 10000;
                            printf(">>>>>>>>>>>>%s\n", parser->current_lr->type);
                        }
                        if(component_format >= 1000){
                            hex_to_number(&len, body, 1);
                            body++;
                            segment_len_remain--;

                            parser->current_lr->name = malloc(len + 1);
                            memmove(parser->current_lr->name, body, len);
                            parser->current_lr->name[len] = '\0';
                            printf(">>>>>>%s\n", parser->current_lr->name);
                            body += len;
                            segment_len_remain -= len;
                        }
                    }
                    break;
            }
            // length = 0;
        }

        length -= current_segment.length;
        //add to logical record
        //break when last logical record segment is read or end or body
        if(current_segment.successor == false){
            parser->current_lr->next = malloc(sizeof(logical_record_t));
            init_logical_record(parser->current_lr->next);
            parser->current_lr = parser->current_lr->next;
        }
    }
}

int lr_read_component_role(unsigned char c){
    int role = c & 224;
    if(role == ABSATR){
        printf("===================================================\n");
    }
    return role;
}

int lr_read_component_format(unsigned char c){
    int format = 0;
    format += c & 16 ? 10000 : 0;
    format += c & 8 ? 1000 : 0;
    format += c & 4 ? 100 : 0;
    format += c & 2 ? 10 : 0;
    format += c & 1 ? 1 : 0;
    return format;
}

//read attributes of an segment
void lr_segment_read_attributes (lr_segment_t * segment, unsigned char c){
    int attributes = c & 255;
    segment->structure = attributes >= 128;
    attributes = attributes >= 128 ? attributes - 128 : attributes;

    segment->predecessor = attributes >= 64;
    attributes = attributes >= 64 ? attributes - 64 : attributes;

    segment->successor = attributes >= 32;
    attributes = attributes >= 32 ? attributes - 32 : attributes;

    segment->encryption = attributes >= 16;
    attributes = attributes >= 16 ? attributes - 16 : attributes;

    segment->encryption_packet = attributes >= 8;
    attributes = attributes >= 8 ? attributes - 8 : attributes;

    segment->checksum = attributes >= 4;
    attributes = attributes >= 4 ? attributes - 4 : attributes;

    segment->trailing_length = attributes >= 2;
    attributes = attributes >= 2 ? attributes - 2 : attributes;

    segment->padding = attributes >= 1;
    printf("lr_segment_read_attributes %d%d%d%d%d%d%d%d\n", segment->structure, segment->predecessor, segment->successor,
        segment->encryption, segment->encryption_packet, segment->checksum, segment->trailing_length, segment->padding);
}

void print_logical_record(parser_t * parser){
    logical_record_t * current = &parser->root_lr;
    while(current != NULL){
        printf(">>>%s_%s\n", current->type, current->name);
        current = current->next;
    }
}

//read logical record template
int lr_read_attribute(logical_record_t* current_lr, int format, unsigned char* data) {
    lr_attribute_t* current_attr;
    if(current_lr->is_template_completed) {
        current_attr = current_lr->current_obj->current_attr;
    } else{
        current_attr = current_lr->current_attribute;
    };

    //init new attribute
    if(current_attr->label != NULL && !current_lr->is_template_completed) {
        current_attr->next = malloc(sizeof(lr_attribute_t));
        current_attr = current_attr->next;
        init_lr_attribute(current_attr);

        //next attribute
        current_lr->current_attribute = current_attr;
    }


    // printf("lr_read_attribute(): format %d\n", format);

    int len = 0;
    int nbytes_read = 0;
    if(format >= 10000){
        //read label
        hex_to_number(&len, data, 1);
        data++;
        nbytes_read++;

        current_attr->label = malloc(len + 1);
        memmove(current_attr->label, data, len);
        current_attr->label[len] = '\0';
        data+=len;
        nbytes_read += len;
        printf("lr_read_attribute(): label %s ", current_attr->label);
        format -= 10000;
    }
    if(format >= 1000){
        //read number of value (count)
        len = uvari_to_int(&current_attr->count, data);
        data+=len;
        nbytes_read+=len;
        format -= 1000;
    }
    if(format >= 100){
        //read presentation code 
        hex_to_number(&current_attr->representation_code, data, 1);
        data++;
        nbytes_read++;
        format -= 100;
        printf("      representation_code %d \n", current_attr->representation_code);
    }
    if(format >= 10){
        //unit
        hex_to_number(&len, data, 1);
        data++;
        nbytes_read++;

        current_attr->unit = malloc(len + 1);
        memmove(current_attr->unit, data, len);
        current_attr->unit[len] = '\0';
        data += len;
        nbytes_read += len;
        format -= 10;
    }
    if(format >= 1 && current_attr->count > 0){
        //read value, the template has 'count' values and the value is present by above representation code 
        len = utils_get_length(current_attr->representation_code) * current_attr->count;
        if(len > 0){
            current_attr->value = malloc(len);
            memmove(current_attr->value, data, len);
            data += len;
            nbytes_read += len;
        } else if(current_attr->representation_code == DLIS_UVARI){
            len = 0;
            for(int i = 0; i < current_attr->count; i++){
                unsigned int tmp_value = 0;
                len += uvari_to_int(&tmp_value, data);
            }
            current_attr->value = malloc(len);
            memmove(current_attr->value, data, len);
            data += len;
            nbytes_read += len;
        } else if (current_attr->representation_code == DLIS_OBNAME) {
            obname_t obj_name;
            init_obname_t(&obj_name);
            int nbytes = utils_read_obname(&obj_name, data);
            data += nbytes;
            nbytes_read += nbytes;
        } else {
            len = 0;
            for(int i = 0; i < current_attr->count; i++){
                unsigned int tmp_value = 0;
                hex_to_number(&tmp_value, data, 1);
                len += tmp_value + 1;
            }
            current_attr->value = malloc(len);
            memmove(current_attr->value, data, len);
            data += len;
            nbytes_read += len;
        }

        printf("lr_read_attribute(): representation_code %d values %s\n", current_attr->representation_code, current_attr->value);
    }
    // printf("lr_read_attribute(): %s_/_%d_/_%d_/_%s_/_%x\n", current_attr->label, current_attr->count, current_attr->representation_code, current_attr->unit, current_attr->value);
    if(current_lr->is_template_completed && current_lr->current_obj->current_attr->next != NULL){
        current_lr->current_obj->current_attr = current_lr->current_obj->current_attr->next;
    }
    return nbytes_read;
}


int lr_read_object(logical_record_t* current_lr, unsigned char* data){
    lr_attribute_t* temp = &current_lr->template;
    printf("\n");
    if(current_lr->current_obj->name != NULL){
        current_lr->current_obj->next = malloc(sizeof(lr_object_t));
        current_lr->current_obj = current_lr->current_obj->next;
        init_lr_object(current_lr->current_obj, &current_lr->template);
    } else {
        init_lr_object(current_lr->current_obj, &current_lr->template);
    }

    obname_t obj_name;
    init_obname_t(&obj_name);
    int nbytes = utils_read_obname(&obj_name, data);

    current_lr->current_obj->origin = obj_name.origin;
    current_lr->current_obj->copy_number = obj_name.copy_number;
    int name_len = strlen((const char *)obj_name.name);
    current_lr->current_obj->name = malloc(name_len + 1);
    memset(current_lr->current_obj->name, name_len + 1, '\0');
    memmove(current_lr->current_obj->name, obj_name.name, name_len);

    printf("lr_read_object() origin %d copy_number %d name: %s\n", current_lr->current_obj->origin, current_lr->current_obj->copy_number, current_lr->current_obj->name);
    return nbytes;
}


int utils_read_obname(obname_t * dest, unsigned char * data){
    int len = 0;
    int nbytes = 0;

    len = uvari_to_int(&dest->origin, data);
    data += len;
    nbytes += len;

    hex_to_number(&dest->copy_number, data, 1);
    data++;

    hex_to_number(&len, data, 1);
    data++;
    nbytes += 2 + len;

    dest->name = malloc(len + 1);
                
    memset(dest->name, len + 1, '\0');
    memmove(dest->name, data, len);
    return nbytes;
}
