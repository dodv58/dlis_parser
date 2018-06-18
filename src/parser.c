 #include "parser.h"

int _count = 0;
// bool is_SUL_parsed = false;

void SUL_init(SUL_t *sul){
    sul->sequence_number = 1;
    memset(&sul->version, '\0', 6);
    memset(&sul->structure, '\0', 7);
    memset(&sul->max_length, '\0', 6);
    memset(&sul->id, '\0', 61);
}

void init_parser(parser_t *parser) {
    SUL_init(&parser->sul);
	cbuffer_init(&parser->cbuffer);
    init_visible_record(&parser->root_vr);
    parser->current_vr = &parser->root_vr;
    init_logical_record(&parser->root_lr);
    parser->current_lr = &parser->root_lr;
    parser->is_SUL_parsed = false;
    parser->current_frame = NULL;
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
    lr->set_type = NULL;
    lr->name = NULL;
    lr->next = NULL;
    lr->code = 0;
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
    // obj->current_attr = &obj->attributes;
    obj->current_attr = NULL;
    obj->next = NULL;

    //copy attributes
    // obj->attributes.representation_code = attr->representation_code;
    
    while(attr != NULL){
        if(obj->current_attr == NULL){
            obj->current_attr = &obj->attributes;
        } else {
            obj->current_attr->next = malloc(sizeof(lr_attribute_t));
            obj->current_attr = obj->current_attr->next;
            init_lr_attribute(obj->current_attr);
        }
        obj->current_attr->label = attr->label;
        obj->current_attr->representation_code = attr->representation_code;
        attr = attr->next;
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
    // print_logical_record(parser);
    print_datasets(parser);
    // print_channels(parser);

}

bool put_data(parser_t *parser,unsigned char *data, int length) {
    print_log("");
    bool insert_status = cbuffer_insert(&parser->cbuffer, data, length);
    if(!insert_status) return insert_status;
    if(!parser->is_SUL_parsed){
        parse_sul(parser);
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

    cbuffer_get(&parser->cbuffer, temp, 6, 4);
    str_to_number(&parser->sul.sequence_number, temp, 4);
    cbuffer_get(&parser->cbuffer, parser->sul.version, 6, 5);
    cbuffer_get(&parser->cbuffer, parser->sul.structure, 7, 6);
    cbuffer_get(&parser->cbuffer, temp, 6, 5);
    str_to_number(&parser->sul.max_length, temp, 5);
    cbuffer_get(&parser->cbuffer, parser->sul.id, 61, 60);

    printf("%d\n", parser->sul.sequence_number);
    printf("%s\n", parser->sul.version);
    printf("%s\n", parser->sul.structure);
    printf("%llu\n", parser->sul.max_length);
    printf("%s\n", parser->sul.id);
    parser->is_SUL_parsed = true;
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
    printf(".................... %d\n", _count);
    _count++;
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
        printf("segment type: %d -- parser->current_lr->code %d \n", current_segment.type, parser->current_lr->code);
        parser->current_lr->code = current_segment.structure == true ? current_segment.type : current_segment.type + 256;

        body++;
        

        // length -= current_segment.length - 4;

        //read segment body
        int segment_len_remain = current_segment.length - 4;
        int lrs_trailer_length = 0;

        if(current_segment.padding) lrs_trailer_length++;
        if(current_segment.checksum) lrs_trailer_length += 2;
        if(current_segment.trailing_length) lrs_trailer_length += 2;
        
        while(segment_len_remain > lrs_trailer_length){
            printf("====>segment_len_remain %d\n", segment_len_remain);
            //read every component
            if(current_segment.structure == 1){
                int component_role = lr_read_component_role(body[0]);
                int component_format = lr_read_component_format(body[0]);
                body++;
                segment_len_remain--;
                
                switch (component_role){
                    case ABSATR: 
                        lr_read_absatr(parser->current_lr);
                        break;
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

                                parser->current_lr->set_type = malloc(len + 1); 
                                memmove(parser->current_lr->set_type, body, len);
                                parser->current_lr->set_type[len] = '\0';
                                body += len;
                                segment_len_remain -= len;
                                component_format -= 10000;
                                printf(">>>>>>>>>>>>%s\n", parser->current_lr->set_type);
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
            } else{
                // obname_t obj_name;
                // init_obname_t(&obj_name);
                // int nbytes = 0;

                // nbytes = utils_read_data(&obj_name, body, DLIS_OBNAME);
                // printf("...........IFLR origin %d copy_number %d obname %s\n",obj_name.origin, obj_name.copy_number, obj_name.name);
                printf("================> %d\n", segment_len_remain);
                lr_parse_iflr(parser, body);
                body += segment_len_remain;
                segment_len_remain = 0;
            }
        }
        body += segment_len_remain;

        length -= current_segment.length;
        //add to logical record
        //break when last logical record segment is read or end or body
        if(current_segment.successor == false){
            if(parser->current_lr->code == 4){
                lr_object_t* obj = &parser->current_lr->objects;
                while(obj != NULL){
                    if(parser->current_frame == NULL){
                        parser->current_frame = &parser->frames;
                    } else {
                        parser->current_frame->next = malloc(sizeof(frame_t));
                        parser->current_frame = parser->current_frame->next;
                    }
                    init_frame_t(parser->current_frame, *obj, parser);
                    obj = obj->next;
                }
            }
            parser->current_lr->next = malloc(sizeof(logical_record_t));
            init_logical_record(parser->current_lr->next);
            parser->current_lr = parser->current_lr->next;
        }
    }
}

int lr_read_component_role(unsigned char c){
    int role = c & 224;
    switch (role){
        case ABSATR: 
            printf("===>ABSATR.    ");
            break;
        case ATTRIB:
            printf("===>ATTRIB.    ");
            break;
        case INVATR:
            printf("===>INVATR.    ");
            break;
        case RDSET:
            printf("===>RDSET.    ");
            break;
        case RSET:
            printf("===>RSET.    ");
            break;        
        case SET:
            printf("===>SET.    ");
            break; 
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
        printf(">>>%s_%s\n", current->set_type, current->name);
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
        len = utils_read_data(&current_attr->count, data, DLIS_UVARI);
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
        len = lr_read_componet_values(current_attr, data);
        data += len;
        nbytes_read += len;
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
    int nbytes = utils_read_data(&obj_name, data, DLIS_OBNAME);

    current_lr->current_obj->origin = obj_name.origin;
    current_lr->current_obj->copy_number = obj_name.copy_number;
    int name_len = strlen((const char *)obj_name.name);
    current_lr->current_obj->name = malloc(name_len + 1);
    memset(current_lr->current_obj->name, name_len + 1, '\0');
    memmove(current_lr->current_obj->name, obj_name.name, name_len);

    printf("lr_read_object() origin %d copy_number %d name: %s\n", current_lr->current_obj->origin, current_lr->current_obj->copy_number, current_lr->current_obj->name);
    return nbytes;
}




void lr_read_absatr(logical_record_t* current_lr){
    if(current_lr->is_template_completed && current_lr->current_obj->current_attr->next != NULL){
        current_lr->current_obj->current_attr = current_lr->current_obj->current_attr->next;
    }
}


int lr_read_componet_values(lr_attribute_t* attr, unsigned char* data){
    int len = 0;
    int nbytes_read = 0;

    len = utils_get_length(attr->representation_code) * attr->count;
    if(len > 0){
        attr->value = malloc(len);
        memmove(attr->value, data, len);
        nbytes_read += len;
    } else if(attr->representation_code == DLIS_UVARI){
        len = 0;
        for(int i = 0; i < attr->count; i++){
            unsigned int tmp_value = 0;
            len += utils_read_data(&tmp_value, data, DLIS_UVARI);
            data += len;
        }
        data -= len;
        attr->value = malloc(len);
        memmove(attr->value, data, len);
        nbytes_read += len;        
    } else if (attr->representation_code == DLIS_OBNAME) {
        len = 0;
        obname_t obj_name;
        init_obname_t(&obj_name);
        int nbytes = 0;

        for(int i = 0; i < attr->count; i++){
            nbytes = utils_read_data(&obj_name, data, DLIS_OBNAME);
            len += nbytes;
            data += nbytes;
        }
        data -= len;
        attr->value = malloc(len);
        memmove(attr->value, data, len);
        nbytes_read += len;
    } else if(attr->representation_code == DLIS_OBJREF){
        unsigned char* type;
        obname_t obname;
        init_obname_t(&obname);
        int nbytes = 0;
        for(int i = 0; i < attr->count; i++){
            nbytes = utils_read_objref(type, &obname, data);
            len += nbytes;
            data += nbytes;
        }
        nbytes_read += len;
    } else {
        len = 0;
        for(int i = 0; i < attr->count; i++){
            unsigned int tmp_value = 0;
            hex_to_number(&tmp_value, data, 1);
            data += tmp_value + 1;
            len += tmp_value + 1;
        }
        data -= len;
        attr->value = malloc(len);
        memmove(attr->value, data, len);
        data += len;
        nbytes_read += len;
    }

    printf("lr_read_attribute(): count %d nbytes %d code %d values %s\n",attr->count, nbytes_read, attr->representation_code, attr->value);
    return nbytes_read;

}

void print_datasets(parser_t *parser){
    printf("=================> datasets:\n");
    frame_t* current = &parser->frames;
    const char * label = "REPRESENTATION-CODE";
    while (current != NULL){
        lr_object_t * obj = &current->frame;
        printf("(%d,%d,%s)\n", obj->origin, obj->copy_number, obj->name );

        for (int i = 0; i < current->number_of_channel; ++i)
        {
            printf("%d_", *(current->representation_codes + i));
        }
        printf("\n");

        lr_object_t * channels = &current->channels;
        while(channels != NULL){
            printf("        (%d,%d,%s) ", channels->origin, channels->copy_number, channels->name );
            lr_attribute_t* attr = &channels->attributes;
            while (attr != NULL && strcmp((const char *) label, (const char *) attr->label) != 0){
                attr = attr->next;
            }
            if(attr == NULL){
                printf("there is no representation_code\n");
                break;
            }
            unsigned int representation_code = 0;
            utils_read_data(&representation_code, attr->value, DLIS_USHORT);
            printf(" representation_code %d\n",representation_code);
            channels = channels->next;
        }
        current = current->next;
    }
}

void print_channels(parser_t *parser){
    printf("=================> channels:\n");
    logical_record_t * current = &parser->root_lr;
    while(current != NULL){
        if(current->code == 3){
            lr_object_t * obj = &current->objects;
            while (obj != NULL){
                printf("(%d,%d,%s)\n", obj->origin, obj->copy_number, obj->name );
                obj = obj->next;
            }
        }
        current = current->next;
    }
}

void init_fdata_t(fdata_t* fdata, int number_of_channel){
    fdata->index = 0;
    fdata->data = malloc(number_of_channel * sizeof(char*));
    // for(int i = 0; i < number_of_channel; i++){
    //     *fdata->data + i = malloc(30 * sizeof(char *));
    // }
    fdata->next = NULL;
}


void init_frame_t(frame_t* frame, lr_object_t frame_obj, parser_t* parser){
    frame->frame = frame_obj;
    frame->current_channel = NULL;
    frame->current_fdata = NULL;

    lr_attribute_t* attr = NULL;
    const char* label = "CHANNELS";
    attr = &frame->frame.attributes;
    while(attr != NULL && strcmp((const char*)attr->label, label) != 0){
        attr = attr->next;
    }
    if(attr == NULL) {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!! there is no CHANNELS attribute\n");
        return;
    }

    logical_record_t * channel_lr = &parser->root_lr;
    while(channel_lr != NULL && channel_lr->code != 3){
        channel_lr = channel_lr->next;
    }

    if(channel_lr == NULL){
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@ channel set is not presented\n");
        return;
    }

    frame->number_of_channel = attr->count;
    obname_t channels[attr->count];
    frame->representation_codes = malloc(sizeof(unsigned int) * frame->number_of_channel);
    unsigned char* p = attr->value;
    for(int i = 0; i < attr->count; i++){
        int nbytes = utils_read_data(&channels[i], p, DLIS_OBNAME);
        p += nbytes;

        lr_object_t * obj = &channel_lr->objects;
        while (obj != NULL){
            if(strcmp((const char*)channels[i].name,(const char *) obj->name) == 0){
                if(frame->current_channel == NULL){
                    frame->current_channel = &frame->channels;
                } else {
                    frame->current_channel->next = malloc(sizeof(lr_object_t));
                    frame->current_channel = frame->current_channel->next;
                }
                memmove(frame->current_channel, obj, sizeof(lr_object_t));

                //read representation code of channel and put to representation_codes of frame
                const char * code_label = "REPRESENTATION-CODE";
                lr_attribute_t* channel_attr = &frame->current_channel->attributes;
                while (channel_attr != NULL && strcmp((const char *) code_label, (const char *) channel_attr->label) != 0){
                    channel_attr = channel_attr->next;
                }
                if(channel_attr == NULL){
                    printf("there is no representation_code\n");
                    break;
                }

                utils_read_data(frame->representation_codes + i, channel_attr->value, DLIS_USHORT);

                frame->current_channel->next = NULL;
                break;
            }
            obj = obj->next;
        }
    }

    init_fdata_t(&frame->fdata, frame->number_of_channel);

    frame->next = NULL;

}

void lr_parse_iflr(parser_t* parser, unsigned char* data){
    obname_t obj_name;
    init_obname_t(&obj_name);
    int nbytes = 0;
    int tmp = 0;

    nbytes = utils_read_data(&obj_name, data, DLIS_OBNAME);
    printf("...........IFLR origin %d copy_number %d obname %s \n",obj_name.origin, obj_name.copy_number, obj_name.name);
    data += nbytes;
    tmp += nbytes;

    //get frame referenced by the IFLR
    frame_t* frame = &parser->frames;
    while(frame != NULL && strcmp((const char*)frame->frame.name, (const char*) obj_name.name) != 0){
        frame = frame->next;
    }
    if(frame == NULL){
        printf("there is no frame %s\n", obj_name.name);
        return;
    }

    if(frame->current_fdata == NULL){
        frame->current_fdata = &frame->fdata;
    } else {
        frame->current_fdata->next = malloc(sizeof(fdata_t));
        init_fdata_t(frame->current_fdata->next, frame->number_of_channel);
        frame->current_fdata = frame->current_fdata->next;
    }

    nbytes = utils_read_data(&frame->current_fdata->index, data, DLIS_UVARI);
    data += nbytes;
    tmp += nbytes;

    for(int i = 0; i < frame->number_of_channel; i++){
        unsigned int representation_code = *(frame->representation_codes + i);
        int len = utils_read_data_to_str(frame->current_fdata->data[i], data, representation_code);
        data += len;
        tmp += len;
    }
    printf("========================> %d\n", tmp);

}
