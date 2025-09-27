#pragma once

template<size_t maxBlendshapes>
BlendshapeController<maxBlendshapes>::BlendshapeController(IEasyEaseAnimator* eEA){
    this->eEA = eEA;
}

template<size_t maxBlendshapes>
void BlendshapeController<maxBlendshapes>::AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset){
    AddBlendshape(dictionaryValue, positionOffset, Vector3D(1.0f, 1.0f, 1.0f), Vector3D());
}

template<size_t maxBlendshapes>
void BlendshapeController<maxBlendshapes>::AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset, Vector3D scaleOffset){
    AddBlendshape(dictionaryValue, positionOffset, scaleOffset, Vector3D());
}

template<size_t maxBlendshapes>
void BlendshapeController<maxBlendshapes>::AddBlendshape(uint16_t dictionaryValue, Vector3D positionOffset, Vector3D scaleOffset, Vector3D rotationOffset){
    if(currentBlendshapes < maxBlendshapes){
        bool addValue = true;
        
        for(uint8_t i = 0; i < currentBlendshapes; i++){
            if(dictionary[i] == dictionaryValue){
                addValue = false;
                break;
            }
        }

        if(addValue){
            positionOffsets[currentBlendshapes] = positionOffset;
            scaleOffsets[currentBlendshapes] = scaleOffset;
            rotationOffsets[currentBlendshapes] = rotationOffset;

            dictionary[currentBlendshapes] = dictionaryValue;
            currentBlendshapes++;
        }
    }
}

template<size_t maxBlendshapes>
void BlendshapeController<maxBlendshapes>::SetBlendshapePositionOffset(uint16_t dictionaryValue, Vector3D positionOffset){
    bool hasIndex = false;
    uint8_t index = 0;
        
    for(uint8_t i = 0; i < currentBlendshapes; i++){
        if(dictionary[i] == dictionaryValue){
            hasIndex = true;
            index = i;
            break;
        }
    }

    if(hasIndex){
        positionOffsets[index] = positionOffset;
    }
}

template<size_t maxBlendshapes>
void BlendshapeController<maxBlendshapes>::SetBlendshapeScaleOffset(uint16_t dictionaryValue, Vector3D scaleOffset){
    bool hasIndex = false;
    uint8_t index = 0;
        
    for(uint8_t i = 0; i < currentBlendshapes; i++){
        if(dictionary[i] == dictionaryValue){
            hasIndex = true;
            index = i;
            break;
        }
    }

    if(hasIndex){
        scaleOffsets[index] = scaleOffset;
    }
}

template<size_t maxBlendshapes>
void BlendshapeController<maxBlendshapes>::SetBlendshapeRotationOffset(uint16_t dictionaryValue, Vector3D rotationOffset){
    bool hasIndex = false;
    uint8_t index = 0;
        
    for(uint8_t i = 0; i < currentBlendshapes; i++){
        if(dictionary[i] == dictionaryValue){
            hasIndex = true;
            index = i;
            break;
        }
    }

    if(hasIndex){
        rotationOffsets[index] = rotationOffset;
    }
}

template<size_t maxBlendshapes>
Vector3D BlendshapeController<maxBlendshapes>::GetPositionOffset(){
    Vector3D positionOffset;
    
    for(uint8_t i = 0; i < currentBlendshapes; i++){
        if (eEA->GetValue(dictionary[i]) > 0.0f){
            positionOffset += positionOffsets[i] * eEA->GetValue(dictionary[i]);
        }
    }
    
    return positionOffset;
}

template<size_t maxBlendshapes>
Vector3D BlendshapeController<maxBlendshapes>::GetScaleOffset(){
    Vector3D scaleOffset = Vector3D(1.0f, 1.0f, 1.0f);
    uint8_t count = 0;
    
    for(uint8_t i = 0; i < currentBlendshapes; i++){
        if (eEA->GetValue(dictionary[i]) > 0.0f){
            scaleOffset = scaleOffset * Vector3D::LERP(Vector3D(1.0f, 1.0f, 1.0f), scaleOffsets[i], eEA->GetValue(dictionary[i]));
            count++;
        }
    }
    
    if(count) return scaleOffset;
    else return Vector3D(1.0f, 1.0f, 1.0f);
}

template<size_t maxBlendshapes>
Vector3D BlendshapeController<maxBlendshapes>::GetRotationOffset(){
    Vector3D rotationOffset;
    
    for(uint8_t i = 0; i < currentBlendshapes; i++){
        if (eEA->GetValue(dictionary[i]) > 0.0f){
            rotationOffset += rotationOffsets[i] * eEA->GetValue(dictionary[i]);
        }
    }
    
    return rotationOffset;
}
