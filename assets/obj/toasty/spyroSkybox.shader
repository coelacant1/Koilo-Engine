
Shader "Custom/spyroSkybox" { //Nom du shader dans la liste des shaders
    Properties {
       //Liste des données parametrables par l'utilisateur en code ou dans l'inspector
    }
    SubShader {
		Cull Off ZWrite Off Fog { Mode off } 
        Pass {
            CGPROGRAM
			#include "UnityCG.cginc" 
            #pragma vertex vertex
            #pragma fragment fragment
			#pragma target 3.0
            
			
			struct Prog2Vertex {
	            float4 vertex : POSITION; 	
	            fixed4 color : COLOR; 
        	 };
			 
			struct Vertex2Pixel
			 {
           	 float4 pos : SV_POSITION;
			 fixed4 color : COLOR; 
			 };  	 
			 
			Vertex2Pixel vertex (Prog2Vertex i)
			{
				Vertex2Pixel o;
		        o.pos = UnityObjectToClipPos (i.vertex); 
				o.color=i.color;				
		      	return o;
			}
            float4 fragment(Vertex2Pixel i) : COLOR 
            {

                return i.color;
            }
            ENDCG
        }
    }
	FallBack "Diffuse" 
}