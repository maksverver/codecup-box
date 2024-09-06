from box import Direction, IsHorizontal

BASE_68_DIGITS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789,-.^_~'

def TileToId(tile):
    '''Encodes tile as an integer between 0 and 720 (exclusive).
The tile must be a valid permutation of the integers 1 through 6 (inclusive).'''
    values = [1, 2, 3, 4, 5, 6]
    id = 0
    for v in tile:
        i = values.index(v)
        id = len(values) * id + i
        del values[i]
    assert not values
    return id

def IdToTile(id):
    '''Inverse of TileToId().'''
    assert id >= 0

    indices = []
    for i in range(6):
        indices.append(id % (i + 1))
        id //= (i + 1)
    assert id == 0
    indices.reverse()

    tile = []
    values = [1, 2, 3, 4, 5, 6]
    for i in indices:
        tile.append(values[i])
        del values[i]
    return tuple(tile)

def PlacementToId(placement):
    '''Encodes placement as an integer between 0 and 434 (exclusive).
The coordinates must be such that a 2x6 tile fits in the grid.'''
    row, col, dir = placement
    if IsHorizontal(dir):
        assert 0 <= row < 15
        assert 0 <= col < 15
        return 15*row + col
    else:
        assert 0 <= row < 11
        assert 0 <= col < 19
        return 15*15 + 19*row + col

def IdToPlacement(id):
    '''Inverse of PlacementToId().'''
    if id < 15 * 15:
        assert id >= 0
        return (id // 15, id % 15, Direction.HORIZONTAL)
    else:
        id -= 15 * 15
        assert id < 11 * 19
        return (id // 19, id % 19, Direction.VERTICAL)

def TilePlacementToId(tile, placement):
    '''Encodes tile and placement as an integer between 0 and 312480 (exclusive).'''
    return 434*TileToId(tile) + PlacementToId(placement)

def IdToTilePlacement(id):
    '''Inverse of IdToPlacement.'''
    assert 0 <= id < 312480
    return IdToTile(id // 434), IdToPlacement(id % 434)

def EncodeSecretColors(colors):
    c1, c2 = colors
    assert 0 <= c1 <= 6
    assert 0 <= c2 <= 6
    return BASE_68_DIGITS[7 * c1 + c2]

def DecodeSecretColors(ch):
    id = BASE_68_DIGITS.index(ch)
    assert 0 <= id < 7 * 7
    return (id // 7, id % 7)

def EncodeTilePlacement(tile, placement):
    id = TilePlacementToId(tile, placement)
    return (BASE_68_DIGITS[id // (68 * 68)] +
            BASE_68_DIGITS[id // 68 % 68] +
            BASE_68_DIGITS[id % 68])

def DecodeTilePlacement(s):
    c1, c2, c3 = s
    id = (BASE_68_DIGITS.index(c1) * 68 * 68 +
          BASE_68_DIGITS.index(c2) * 68 +
          BASE_68_DIGITS.index(c3))
    return IdToTilePlacement(id)

def test_TileToId():
    assert TileToId((1,2,3,4,5,6)) == 0
    assert TileToId((2,1,3,6,5,4)) == 125
    assert TileToId((6,5,4,3,2,1)) == 719

def test_IdToTile():
    assert IdToTile(0) == (1,2,3,4,5,6)
    assert IdToTile(125) == (2,1,3,6,5,4)
    assert IdToTile(719) == (6,5,4,3,2,1)

def test_PlacementToId():
    assert PlacementToId(( 0,  0, Direction.HORIZONTAL)) ==   0
    assert PlacementToId(( 3,  6, Direction.HORIZONTAL)) ==  51
    assert PlacementToId((14, 14, Direction.HORIZONTAL)) == 224
    assert PlacementToId(( 0,  0, Direction.VERTICAL))   == 225
    assert PlacementToId(( 3,  6, Direction.VERTICAL))   == 288
    assert PlacementToId((10, 18, Direction.VERTICAL))   == 433

def test_IdToPlacement():
    assert IdToPlacement(  0) == (( 0,  0, Direction.HORIZONTAL))
    assert IdToPlacement( 51) == (( 3,  6, Direction.HORIZONTAL))
    assert IdToPlacement(224) == ((14, 14, Direction.HORIZONTAL))
    assert IdToPlacement(225) == (( 0,  0, Direction.VERTICAL))
    assert IdToPlacement(288) == (( 3,  6, Direction.VERTICAL))
    assert IdToPlacement(433) == ((10, 18, Direction.VERTICAL))

def test_TilePlacementToId():
    assert TilePlacementToId((1, 2, 3, 4, 5, 6), ( 0,  0, Direction.HORIZONTAL)) == 0
    assert TilePlacementToId((3, 1, 4, 5, 6, 2), ( 7, 14, Direction.VERTICAL)) == 108438
    assert TilePlacementToId((6, 5, 4, 3, 2, 1), (10, 18, Direction.VERTICAL)) == 312479

def test_IdToTilePlacement():
    assert IdToTilePlacement(0) == ((1, 2, 3, 4, 5, 6), ( 0,  0, Direction.HORIZONTAL))
    assert IdToTilePlacement(108438) == ((3, 1, 4, 5, 6, 2), (7, 14, Direction.VERTICAL))
    assert IdToTilePlacement(312479) == ((6, 5, 4, 3, 2, 1), (10, 18, Direction.VERTICAL))

def test_EncodeSecretColors():
    assert EncodeSecretColors((0, 0)) == 'A'
    assert EncodeSecretColors((0, 1)) == 'B'
    assert EncodeSecretColors((4, 2)) == 'e'
    assert EncodeSecretColors((6, 6)) == 'w'

def test_DecodeSecretColors():
    assert DecodeSecretColors('A') == (0, 0)
    assert DecodeSecretColors('B') == (0, 1)
    assert DecodeSecretColors('e') == (4, 2)
    assert DecodeSecretColors('w') == (6, 6)

def test_EncodeTilePlacement():
    assert EncodeTilePlacement((1, 2, 3, 4, 5, 6), ( 0,  0, Direction.HORIZONTAL)) == 'AAA'
    assert EncodeTilePlacement((3, 1, 4, 5, 6, 2), ( 7, 14, Direction.VERTICAL)) == 'Xeu'
    assert EncodeTilePlacement((6, 5, 4, 3, 2, 1), (10, 18, Direction.VERTICAL)) == '~nT'

def test_DecodeTilePlacement():
    assert DecodeTilePlacement('AAA') == ((1, 2, 3, 4, 5, 6), ( 0,  0, Direction.HORIZONTAL))
    assert DecodeTilePlacement('Xeu') == ((3, 1, 4, 5, 6, 2), ( 7, 14, Direction.VERTICAL))
    assert DecodeTilePlacement('~nT') == ((6, 5, 4, 3, 2, 1), (10, 18, Direction.VERTICAL))
