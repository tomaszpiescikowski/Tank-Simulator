#version 330


uniform sampler2D tex;

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela

//Zmienne interpolowane
in float i_nl;
in vec2 i_tc;

void main(void) {
    //vec4 color=texture(tex,i_tc);	//tymczasowo zakomentowane
	vec4 color=vec4(0.7, 0.7, 0.7, 1.0);
	pixelColor=vec4(color.rgb*i_nl,color.a);
}
