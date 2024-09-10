//Caso o jogador oponente consiga interceptar a bola returna 1,
//caso contrário returna 0(este valor será subtraido no peso Geral)
//(x1,y1)Coord. Jogador a tocar a bola; (x2,y2)Coord. Jogador a receber a bola;
int Player::tempoInterceptacao(double x1, double y1, double x2, double y2)
{
    double a, b, b2,x3,y3,pontx, ponty, dist_mate, dist_opponent=100, tempoad_aux=50,tempob_aux=0,tempob, tempoad, veloc_b=2.7, veloc_ad=1.0;
    ObjectT Opponents[10] ={OBJECT_OPPONENT_2,OBJECT_OPPONENT_3,OBJECT_OPPONENT_4,OBJECT_OPPONENT_5,OBJECT_OPPONENT_6,OBJECT_OPPONENT_7,OBJECT_OPPONENT_8,OBJECT_OPPONENT_9,OBJECT_OPPONENT_10,OBJECT_OPPONENT_11};

    if(x1==x2)  //Corrige erro de divisão por 0, pois nao e possivel definir eq. da reta
        x2 += 0.001; //Para deixar o x1 diferente do x2.

    if(y1==y2) //Corrige erro de divisão por 0, pois a componente 'a' da eq resulta em 0 e impossibilitando calcular seu inverso para o calculo da perpendicular
        y2 += 0.001; //Para deixar o y1 diferente do y2

    a = (y2-y1)/(x2-x1); //Encontrando a componente 'a' da eq.
    b = y1 - a*x1; //Encontrando a componente 'b' da eq.
    //Obtem-se a equação reduzida da reta (y=ax+b)


    //Verificando qual é o jogador oponente mais rápido ao ponto de interceptação.
    for(int i = 0; i < 11; i++)
    {
        //(x3,y3)Coord. Jogador a roubar a bola
        x3 = WM->getGlobalPosition(Opponents[i]).getX();
        y3 = WM->getGlobalPosition(Opponents[i]).getY();

        b2 = y3+1/a*x3; //Encontrando a componente 'b2' da eq. da reta perpendicular formada pelo jogador oponente e ponto de interceptação

        pontx = (b-b2)/((-1/a)-a); //Ponto x que as retas se interceptam
        ponty = (a*pontx) +b; //ponto y onde as retas se interceptam

        dist_mate = sqrt(pow(pontx-x1,2)+pow(ponty-y1,2)); //Distância do jogador que irá tocar até o ponto de interceptação.
        dist_opponent = sqrt(pow(pontx-x3,2)+pow(ponty-y3,2)); //Distâcia do oponente até o ponto de interceptação.


        if(x1<x2)  //Verificar se a componente x do jogador a tocar a bola é menor que a do companheiro a receber a bola
        {
            if(pontx>x1 && pontx<x2)  //Se o ponto de interceptação estiver no segmento de reta(trajetória da bola)
            {
                tempob = dist_mate/veloc_b; //Calcula o tempo que a bola gasta para chegar ao ponto de interceptação
                tempoad = dist_opponent/veloc_ad; //Calcula o tempo que o oponente gasta para chegar ao ponto de interceptação

            }
            //Se o ponto de interceptação não estiver entre o intervalo indicado,
            //o jogador só consegue roubar se estiver depois do jogador a receber,
            //caso contrário não é possível.
            else
            {
                //Se o jogador oponente estiver depois do jogador a receber a bola,
                //a distância não será mais com relação ao ponto de interceptação,
                //mas sim com relação ao jogador a receber a bola.
                if(pontx>x2)
                {
                    dist_opponent = sqrt(pow(x2-x3,2)+pow(y2-y3,2));
                    dist_mate = sqrt(pow(x2-x1,2)+pow(y2-y1,2));

                    tempob = dist_mate/veloc_b;
                    tempoad = dist_opponent/veloc_ad;
                }
                else
                {
                    tempoad = 100; //Este adversário não consegue roubar a bola por estar atrás do jogador a tocar a bola
                }

            }
        }
        else
        {
            if(pontx<x1 && pontx>x2)
            {
                tempob = dist_mate/veloc_b;
                tempoad = dist_opponent/veloc_ad;

            }
            else
            {
                if(pontx<x2)
                {
                    dist_opponent = sqrt(pow(x2-x3,2)+pow(y2-y3,2));
                    dist_mate = sqrt(pow(x2-x1,2)+pow(y2-y1,2));

                    tempob = dist_mate/veloc_b;
                    tempoad = dist_opponent/veloc_ad;
                }
                else
                {
                    tempoad = 100;
                }
            }
        }

        //Verifica o adversário que está mais perto pra roubar a bola.
        if(tempoad<tempoad_aux)
        {
            tempoad_aux = tempoad;
            tempob_aux = tempob;
        }
    }//Fecha o for


    if(tempob_aux<tempoad_aux)
        return 0;
    else
        return 1;

} //Finaliza o método tempoInterceptacao






//Retorna um VecPosition do melhor jogador que está na REGIÃO 1 para tocar a bola,
//recebe como parametro o jogador a tocar a bola
VecPosition Player::melhorJogadorRegiao1 (ObjectT Companheiro)
{
    ObjectT mates[10]; //Vetor de companheiros que estão na região 1
    ObjectT Teammates[10] = {OBJECT_TEAMMATE_2,OBJECT_TEAMMATE_3,OBJECT_TEAMMATE_4,OBJECT_TEAMMATE_5,OBJECT_TEAMMATE_6,OBJECT_TEAMMATE_7,OBJECT_TEAMMATE_8,OBJECT_TEAMMATE_9,OBJECT_TEAMMATE_10,OBJECT_TEAMMATE_11};
    int number_mates = 0; // Numero de companheiros na região 1
    int indice_jogador; //Indice do melhor peso de toque, com isso obter a posição desse jogador

    double T[10]; //T > Peso do toque
    double aux=0;
    double Pi=0; // peso da posicao de i
    double ad=0; //Ad >  Peso de posição dos adversários com relação a um adversário interceptar a bola
    double dist = 10; //Distância ideal
    double di; //Distância relativa entre o agente e o jogador a receber a bola

    // Verifica quais companheiros estão na região 1, estes jogadores são colocados no vetor de objetos;
    for(int i=0; i<11; i++)
    {

        if(getRegion(Teammates[i]) == 1)
        {
            if(Teammates[i]!=Companheiro) //O jogador a tocar a bola não será add no vetor de companheiros
            {
            mates[number_mates] = Teammates[i];
            number_mates++;
            }else
                continue;
        }
    }

    //Irá verificar o peso de todos os jogadores que estão na Região 1
    for (int i = 0; i < number_mates; i++)
    {


        di = WM->getRelativeDistance(mates[i]);

        if (di <= dist) // if para d menores que a d ideal
			Pi = di / dist;

		else if (di > dist) // if para d maiores que a d ideal
			Pi = (2 * dist - di) / dist;


        ad = tempoInterceptacao(WM->getGlobalPosition(Companheiro).getX(),WM->getGlobalPosition(Companheiro).getY(),
                                WM->getGlobalPosition(mates[i]).getX(),WM->getGlobalPosition(mates[i]).getY());

        T[i] =  Pi - ad;

        if (T[i]>=aux)
        {
            aux = T[i]; //Double auxiliar recebe o valor do maior peso
            indice_jogador = i; //Obtem o indice do vetor de companheiros, para saber qual é o jogador com melhor peso
        }


    } //Fecha o for

    VecPosition pos_jogador = WM->getGlobalPosition(mates[indice_jogador]); // Posição do melhor jogador
    return pos_jogador; //Retorna a posição global do melhor jogador para tocar a bola.
} //Finaliza o método melhorJogadorRegiao1
