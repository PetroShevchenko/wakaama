#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <wiringPi.h>  
#include <softPwm.h>

enum {
    RED_PIN = 8,    // GPIO 2
    GREEN_PIN = 9,  // GPIO 3
    BLUE_PIN = 7    // GPIO 4
};

enum {
    LIGHT_CONTROL_INSTANCE_PARAMETERS = 8
};

typedef struct light_control_instance_s {
    struct light_control_instance_s * next;
    uint16_t short_id;
    bool state;
    uint8_t dimmer;
    uint32_t on_time;
    double comulative_active_power;
    double power_factor;
    const char * colour;
    const char * sensor_units;
    const char * application_type;
}light_control_instance_t;

static bool prv_light_control_switch(light_control_instance_t *instance)
{
    if (instance == NULL) return false;
    int pin;
    uint8_t dimmer = instance->state ? 100 - instance->dimmer : 100;
    if (strncmp(instance->colour, "RED", 3) == 0)
        pin = RED_PIN;
    else if (strncmp(instance->colour, "GREEN", 5) == 0)
        pin = GREEN_PIN;
    else if (strncmp(instance->colour, "BLUE", 4) == 0)
        pin = BLUE_PIN;
    else return false;

    softPwmWrite(pin, dimmer);
    return true;
}

static bool prv_light_control_set_colour(lwm2m_data_t * data, light_control_instance_t *instance)
{
    if (dataArray == NULL || instance == NULL) return false;
    if (strncmp(data, "RED", 3) == 0)
        instance->colour = "RED";
    else if (strncmp(data, "GREEN", 5) == 0)
        instance->colour = "GREEN";
    else if (strncmp(data, "BLUE", 4) == 0)
        instance->colour = "BLUE";                
    else
        return false;
    return true;
}

static uint8_t prv_light_control_read(uint16_t instanceId,
                                 int * numDataP,
                                 lwm2m_data_t ** dataArrayP,
                                 lwm2m_object_t * objectP)
{
    int i;
    uint8_t result;
    light_control_instance_t * targetP;
    
    targetP = (light_control_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;   

    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_data_new(LIGHT_CONTROL_INSTANCE_PARAMETERS);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = LIGHT_CONTROL_INSTANCE_PARAMETERS;
        (*dataArrayP)[0].id = 5850;
        (*dataArrayP)[1].id = 5851;
        (*dataArrayP)[2].id = 5852;
        (*dataArrayP)[3].id = 5805;
        (*dataArrayP)[4].id = 5820;
        (*dataArrayP)[5].id = 5706;
        (*dataArrayP)[6].id = 5701;
        (*dataArrayP)[7].id = 5750;
    }

    for (i = 0 ; i < *numDataP ; i++)
    {
        switch ((*dataArrayP)[i].id)
        {
        case 5850:
            lwm2m_data_encode_bool(targetP->state, *dataArrayP + i);
            break;
        case 5851:
            lwm2m_data_encode_uint(targetP->dimmer, *dataArrayP + i);
            break;
        case 5852:
            lwm2m_data_encode_uint(targetP->on_time, *dataArrayP + i);
            break;
        case 5805:
            lwm2m_data_encode_float(targetP->comulative_active_power, *dataArrayP + i);
            break;
        case 5820:
            lwm2m_data_encode_float(targetP->power_factor, *dataArrayP + i);
            break;
        case 5706:
            lwm2m_data_encode_string(targetP->colour, *dataArrayP + i);
            break;
        case 5701:
            lwm2m_data_encode_string(targetP->sensor_units, *dataArrayP + i);
            break;
        case 5750:
            lwm2m_data_encode_string(targetP->application_type, *dataArrayP + i);
            break;
            
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}
static uint8_t prv_light_control_discover(uint16_t instanceId,
                            int * numDataP,
                            lwm2m_data_t ** dataArrayP,
                            lwm2m_object_t * objectP)
{
    int i;

    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_data_new(LIGHT_CONTROL_INSTANCE_PARAMETERS);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = LIGHT_CONTROL_INSTANCE_PARAMETERS;
        (*dataArrayP)[0].id = 5850;
        (*dataArrayP)[1].id = 5851;
        (*dataArrayP)[2].id = 5852;
        (*dataArrayP)[3].id = 5805;
        (*dataArrayP)[4].id = 5820;
        (*dataArrayP)[5].id = 5706;
        (*dataArrayP)[6].id = 5701;
        (*dataArrayP)[7].id = 5750;
    }
    else
    {
        for (i = 0; i < *numDataP; i++)
        {
            switch ((*dataArrayP)[i].id)
            {
            case 5850:
            case 5851:
            case 5852:
            case 5805:
            case 5820:
            case 5706:
            case 5701:
            case 5750:
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }
    }

    return COAP_205_CONTENT;
}
static uint8_t prv_light_control_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    light_control_instance_t * targetP;
    int i;
    FILE * fp;

    targetP = (light_control_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    for (i = 0 ; i < numData ; i++)
    {
        switch (dataArray[i].id)
        {
        case 5850:
        {
            bool value;

            if (1 != lwm2m_data_decode_bool(dataArray + i, &value))
            {
                return COAP_400_BAD_REQUEST;
            }
            targetP->state = value;
            if (!prv_light_control_switch(targetP)) return COAP_500_INTERNAL_SERVER_ERROR;
        }
        break;
        case 5851:
        {
            uint8_t value;

            if (1 != lwm2m_data_decode_uint(dataArray + i, &value) || value > 100)
            {
                return COAP_400_BAD_REQUEST;
            }
            targetP->dimmer = value;
        }
        break;
        case 5706:
        {
            if (!prv_light_control_set_colour(dataArray + i, targetP))
            {
                return COAP_400_BAD_REQUEST;
            }
        }
        break;
        case 5852:
        case 5805:
        case 5820:
        case 5701:
        case 5750:
            return COAP_405_METHOD_NOT_ALLOWED;

        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_204_CHANGED;
}

lwm2m_object_t * get_light_control_object(void)
{
    lwm2m_object_t * lightCtrlObj;

    lightCtrlObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != lightCtrlObj)
    {
        int i;
        light_control_instance_t * targetP;

        memset(lightCtrlObj, 0, sizeof(lwm2m_object_t));

        lightCtrlObj->objID = LIGHT_CONTROL_OBJECT_ID;

        targetP = (light_control_instance_t *)lwm2m_malloc(sizeof(light_control_instance_t));
        if (NULL == targetP) return NULL;
        memset(targetP, 0, sizeof(light_control_instance_t));

        if (wiringPiSetup() == -1) return NULL;
        softPwmCreate(RED_PIN, 100, 100);
        softPwmCreate(GREEN_PIN, 100, 100);
        softPwmCreate(BLUE_PIN, 100, 100);

        targetP->short_id = 0;
        targetP->state = false;
        targetP->dimmer =100;
        targetP->on_time = 0;
        targetP->comulative_active_power = 0.0;
        targetP->power_factor = 0.0;
        targetP->colour = "RED";
        targetP->sensor_units = "light-%";
        targetP->application_type = "RGB_LED";
        lightCtrlObj->instanceList = LWM2M_LIST_ADD(lightCtrlObj->instanceList, targetP);


        lightCtrlObj->readFunc = prv_light_control_read;
        lightCtrlObj->discoverFunc = prv_light_control_discover;
        lightCtrlObj->writeFunc = prv_light_control_write;
        lightCtrlObj->executeFunc = NULL;
        lightCtrlObj->createFunc = NULL;
        lightCtrlObj->deleteFunc = NULL;
    }

    return lightCtrlObj;
}

void free_light_control_object(lwm2m_object_t * object)
{
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        lwm2m_free(object->userData);
        object->userData = NULL;
    }
    lwm2m_free(object);
}

void display_light_control_object(lwm2m_object_t * objectP)
{

}

