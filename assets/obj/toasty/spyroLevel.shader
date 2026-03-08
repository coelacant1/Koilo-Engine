
Shader "Custom/spyroLevel" { //Nom du shader dans la liste des shaders
    Properties {
       //Liste des données parametrables par l'utilisateur en code ou dans l'inspector
		_MainTex("Diffuse RGBA", 2D)= "white"{}
    }
    SubShader {
        Pass {
            CGPROGRAM
			#include "UnityCG.cginc" 
            #pragma vertex vertex
            #pragma fragment fragment
			#pragma target 3.0
            
			sampler2D _MainTex;
			
			struct Prog2Vertex {
	            float4 vertex : POSITION; 	
	            float4 texcoord : TEXCOORD0;  
	            fixed4 color : COLOR; 
        	 };
			 
			struct Vertex2Pixel
			 {
           	 float4 pos : SV_POSITION;
           	 float4 uv : TEXCOORD0;
			 fixed4 color : COLOR; 
			 };  	 
			 
			Vertex2Pixel vertex (Prog2Vertex i)
			{
				Vertex2Pixel o;
		        o.pos = UnityObjectToClipPos (i.vertex); 
		        o.uv=i.texcoord; 
				o.color=i.color;				
		      	return o;
			}
            float4 fragment(Vertex2Pixel i) : COLOR 
            {
				float4 tempTex=tex2D(_MainTex,i.uv.xy);
                return i.color*tempTex*1.6; //1.6 is an empirically found value used to boost the lighting as in the original game
            }
            ENDCG
        }
    }
	FallBack "Diffuse" 
}